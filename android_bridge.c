/*
 * android_bridge.c — EBC Android output bridge for rtl_433
 *
 * Implements a custom data_output_t that serialises decoded sensor events to
 * JSON strings and delivers them to the Kotlin layer via the JNI callbacks
 * announce_output_line() and announce_device_stat() (defined in rtlsdr433.cpp).
 *
 * No stdout/stderr redirection is used.  The output goes directly from the
 * rtl_433 data pipeline into the JNI bridge.
 *
 * Pipeline:
 *   rtl_433 decoder -> data_acquired_handler() -> data_output_android_print()
 *   -> json_buf -> announce_output_line()
 *   -> rtlsdr433.cpp (JNI) -> NativeBridge.nativeOutputLine()
 *   -> MyService -> LiveJsonStore -> SensorRepository -> Compose UI
 *
 * Copyright (C) 2025 Christian Ebner <info@ebctech.eu>
 *
 * This file is a derived work of rtl_433 (https://github.com/merbanan/rtl_433),
 * which is licensed under the GNU General Public License version 2 (GPL-2.0).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "rtl_433.h"
#include "data.h"
#include "r_api.h"
#include "list.h"

#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "AndroidBridge"

/* Forward declarations for callbacks implemented in rtlsdr433.cpp */
extern void announce_output_line(const char *line);
extern void announce_device_stat(int dev_state);

/* -------------------------------------------------------------------------
 * JSON serialisation helpers (minimal, no external dependencies)
 * ---------------------------------------------------------------------- */

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} json_buf_t;

static void jb_init(json_buf_t *jb)
{
    jb->cap = 512;
    jb->buf = malloc(jb->cap);
    jb->len = 0;
    if (jb->buf) {
        jb->buf[0] = '\0';
    } else {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG, "jb_init: malloc failed");
        jb->cap = 0;
    }
}

static void jb_free(json_buf_t *jb)
{
    free(jb->buf);
    jb->buf = NULL;
    jb->len = 0;
    jb->cap = 0;
}

static void jb_ensure(json_buf_t *jb, size_t need)
{
    if (!jb->buf) return;
    if (jb->len + need + 1 > jb->cap) {
        size_t new_cap = (jb->len + need + 1) * 2;
        char *new_buf = realloc(jb->buf, new_cap);
        if (!new_buf) {
            __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,
                    "jb_ensure: realloc failed (needed %zu bytes)", new_cap);
            free(jb->buf);
            jb->buf = NULL;
            jb->cap = 0;
            return;
        }
        jb->buf = new_buf;
        jb->cap = new_cap;
    }
}

static void jb_append(json_buf_t *jb, const char *s)
{
    if (!s || !jb->buf) return;
    size_t sl = strlen(s);
    jb_ensure(jb, sl);
    if (jb->buf) {
        memcpy(jb->buf + jb->len, s, sl + 1);
        jb->len += sl;
    }
}

static void jb_append_char(json_buf_t *jb, char c)
{
    jb_ensure(jb, 1);
    if (jb->buf) {
        jb->buf[jb->len++] = c;
        jb->buf[jb->len]   = '\0';
    }
}

/* Append a JSON-escaped string (with surrounding quotes) */
static void jb_append_str(json_buf_t *jb, const char *s)
{
    if (!s) { jb_append(jb, "null"); return; }
    jb_append_char(jb, '"');
    for (; *s; s++) {
        if (*s == '"' || *s == '\\' || *s == '/') {
            jb_append_char(jb, '\\');
            jb_append_char(jb, *s);
        } else if (*s == '\n') {
            jb_append(jb, "\\n");
        } else if (*s == '\r') {
            jb_append(jb, "\\r");
        } else if (*s == '\t') {
            jb_append(jb, "\\t");
        } else {
            jb_append_char(jb, *s);
        }
    }
    jb_append_char(jb, '"');
}

/* Forward declaration */
static void data_to_json(json_buf_t *jb, data_t *data);

static void array_to_json(json_buf_t *jb, data_array_t *arr)
{
    if (!arr || !arr->values) { jb_append(jb, "[]"); return; }
    jb_append_char(jb, '[');
    for (int i = 0; i < arr->num_values; i++) {
        if (i > 0) jb_append_char(jb, ',');
        switch (arr->type) {
        case DATA_STRING:
            jb_append_str(jb, ((char **)arr->values)[i]);
            break;
        case DATA_INT: {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", ((int *)arr->values)[i]);
            jb_append(jb, tmp);
            break;
        }
        case DATA_DOUBLE: {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%g", ((double *)arr->values)[i]);
            jb_append(jb, tmp);
            break;
        }
        case DATA_DATA:
            data_to_json(jb, ((data_t **)arr->values)[i]);
            break;
        default:
            jb_append(jb, "null");
            break;
        }
    }
    jb_append_char(jb, ']');
}

static void data_to_json(json_buf_t *jb, data_t *data)
{
    if (!data) { jb_append(jb, "null"); return; }
    jb_append_char(jb, '{');
    int first = 1;
    for (data_t *d = data; d; d = d->next) {
        if (d->type == DATA_COND) continue;
        if (!first) jb_append_char(jb, ',');
        first = 0;
        jb_append_str(jb, d->key);
        jb_append_char(jb, ':');
        switch (d->type) {
        case DATA_STRING:
            jb_append_str(jb, (const char *)d->value.v_ptr);
            break;
        case DATA_INT: {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", d->value.v_int);
            jb_append(jb, tmp);
            break;
        }
        case DATA_DOUBLE: {
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "%g", d->value.v_dbl);
            jb_append(jb, tmp);
            break;
        }
        case DATA_ARRAY:
            array_to_json(jb, (data_array_t *)d->value.v_ptr);
            break;
        case DATA_DATA:
            data_to_json(jb, (data_t *)d->value.v_ptr);
            break;
        default:
            jb_append(jb, "null");
            break;
        }
    }
    jb_append_char(jb, '}');
}

/* -------------------------------------------------------------------------
 * Custom data_output_t implementation
 * ---------------------------------------------------------------------- */

typedef struct {
    data_output_t output; /* must be first */
} data_output_android_t;

static void R_API_CALLCONV android_output_print(data_output_t *output,
        data_t *data)
{
    (void)output;
    if (!data) return;

    json_buf_t jb;
    jb_init(&jb);
    data_to_json(&jb, data);

    if (jb.buf && jb.len > 0) {
        __android_log_print(ANDROID_LOG_INFO, LOG_TAG,
                "android_output_print: dispatching JSON len=%zu: %.200s",
                jb.len, jb.buf);
        announce_output_line(jb.buf);
    } else {
        __android_log_write(ANDROID_LOG_WARN, LOG_TAG,
                "android_output_print: empty JSON buffer — data not serialised");
    }

    jb_free(&jb);
}

static void R_API_CALLCONV android_output_start(data_output_t *output,
        char const * const *fields, int num_fields)
{
    (void)output;
    (void)fields;
    (void)num_fields;
    /* nothing to initialise */
}

static void R_API_CALLCONV android_output_free(data_output_t *output)
{
    free(output);
}

/* -------------------------------------------------------------------------
 * Public API: register the Android output handler with rtl_433 config
 * ---------------------------------------------------------------------- */

void android_bridge_add_output(r_cfg_t *cfg)
{
    data_output_android_t *out = calloc(1, sizeof(data_output_android_t));
    if (!out) {
        __android_log_write(ANDROID_LOG_ERROR, LOG_TAG,
                "android_bridge_add_output: calloc failed");
        return;
    }

    out->output.output_print = android_output_print;
    out->output.output_start = android_output_start;
    out->output.output_free  = android_output_free;

    list_push(&cfg->output_handler, out);

    __android_log_write(ANDROID_LOG_DEBUG, LOG_TAG,
            "Android JSON output handler registered");
}
