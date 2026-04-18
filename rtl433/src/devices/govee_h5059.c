/** @file
    Govee Water Leak Detector H5059.

    Copyright (C) 2026 Reece Neff <reeceneff@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "decoder.h"

#define GOVEE_H5059_SYNC_LEN            3
#define GOVEE_H5059_MIN_FRAME           7
#define GOVEE_H5059_MAX_FRAME           128
#define GOVEE_H5059_KEY_LEN             128
#define GOVEE_H5059_MIN_DECRYPTED_LEN   19
#define GOVEE_H5059_CRC_POLY            0x1021
#define GOVEE_H5059_CRC_INIT            0x1d0f
#define GOVEE_H5059_MSG_CLASS_OFFSET    0
#define GOVEE_H5059_ID_OFFSET           1
#define GOVEE_H5059_SUBTYPE_OFFSET      13
#define GOVEE_H5059_LEAK_FLAG1_OFFSET   15
#define GOVEE_H5059_LEAK_FLAG2_OFFSET   17
#define GOVEE_H5059_LEAK_FLAG_ACTIVE    0x01
#define GOVEE_H5059_MSG_CLASS_TELEMETRY 0x11
#define GOVEE_H5059_MSG_CLASS_PAIRING   0x01
#define GOVEE_H5059_MSG_CLASS_OTHER     0x02
#define GOVEE_H5059_SUBTYPE_BUTTON      0x05
#define GOVEE_H5059_SUBTYPE_LEAK        0x06
#define GOVEE_H5059_SUBTYPE_POST_ALARM  0x07

static uint8_t const govee_h5059_sync[]       = {0x2c, 0x4c, 0x4a};
static uint8_t const govee_h5059_sync_skew1[] = {0x16, 0x26, 0x25};
static uint8_t const govee_h5059_key[GOVEE_H5059_KEY_LEN + 1] =
        "s6amyEvO8UslCY0eZjgc2S6APCVLgLxzFvL2Z5GWPW7fKVjy2oAU6uiKU3lZCHm62VYQQuCtgxzPgGd8UDRPVZpDRAsh5EdYq1E4j4morJ3vd6tWx8BiWOLDc2I8wKUK";

enum {
    GOVEE_H5059_LEAK_UNKNOWN = -1,
    GOVEE_H5059_LEAK_DRY     = 0,
    GOVEE_H5059_LEAK_WET     = 1,
};

/**
Govee Water Leak Detector H5059.
*/
static int govee_h5059_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    uint8_t frame[GOVEE_H5059_MAX_FRAME];
    uint8_t dec[64];

    int row           = -1;
    unsigned sync_pos = 0;

    for (int r = 0; r < bitbuffer->num_rows; ++r) {
        if (bitbuffer->bits_per_row[r] < 8 * GOVEE_H5059_MIN_FRAME)
            continue;

        unsigned pos = bitbuffer_search(bitbuffer, r, 0, govee_h5059_sync, GOVEE_H5059_SYNC_LEN * 8);
        if (pos < bitbuffer->bits_per_row[r]) {
            row      = r;
            sync_pos = pos;
            break;
        }

        unsigned skew_pos = bitbuffer_search(bitbuffer, r, 0, govee_h5059_sync_skew1, GOVEE_H5059_SYNC_LEN * 8);
        if (skew_pos < bitbuffer->bits_per_row[r]) {
            row      = r;
            sync_pos = skew_pos + 1;
            break;
        }
    }

    if (row < 0)
        return DECODE_ABORT_EARLY;

    sync_pos += GOVEE_H5059_SYNC_LEN * 8;

    unsigned bits_after_sync = bitbuffer->bits_per_row[row] - sync_pos;
    if (bits_after_sync < 8 * 4)
        return DECODE_ABORT_LENGTH;

    unsigned bytes_after_sync = bits_after_sync / 8;
    if (bytes_after_sync > GOVEE_H5059_MAX_FRAME)
        bytes_after_sync = GOVEE_H5059_MAX_FRAME;

    bitbuffer_extract_bytes(bitbuffer, row, sync_pos, frame, bytes_after_sync * 8);

    uint8_t outer_len = frame[0];
    if (outer_len < 4 || outer_len > GOVEE_H5059_MAX_FRAME - 1)
        return DECODE_FAIL_SANITY;

    if (bytes_after_sync < (unsigned)(1 + outer_len))
        return DECODE_ABORT_LENGTH;

    uint8_t seed      = frame[1];
    unsigned enc_len  = outer_len - 3;
    unsigned crc_offs = 2 + enc_len;

    if (enc_len < 8 || enc_len > sizeof(dec))
        return DECODE_FAIL_SANITY;

    uint16_t crc_calc = crc16(&frame[2], enc_len, GOVEE_H5059_CRC_POLY, GOVEE_H5059_CRC_INIT);
    uint16_t crc_recv = ((uint16_t)frame[crc_offs] << 8) | frame[crc_offs + 1];
    if (crc_calc != crc_recv)
        return DECODE_FAIL_MIC;

    for (unsigned i = 0; i < enc_len; ++i)
        dec[i] = frame[2 + i] ^ govee_h5059_key[(i + seed) % GOVEE_H5059_KEY_LEN];

    if (enc_len < GOVEE_H5059_MIN_DECRYPTED_LEN)
        return DECODE_FAIL_SANITY;

    uint8_t msg_class = dec[GOVEE_H5059_MSG_CLASS_OFFSET];
    uint32_t id_wire  = ((uint32_t)dec[1] << 24) | ((uint32_t)dec[2] << 16) | ((uint32_t)dec[3] << 8) | dec[4];
    uint32_t id = ((id_wire & 0xffff) << 16) | ((id_wire >> 16) & 0xffff);

    int subtype     = enc_len > GOVEE_H5059_SUBTYPE_OFFSET ? dec[GOVEE_H5059_SUBTYPE_OFFSET] : -1;
    int leak_flag_1 = enc_len > GOVEE_H5059_LEAK_FLAG1_OFFSET ? dec[GOVEE_H5059_LEAK_FLAG1_OFFSET] : -1;
    int leak_flag_2 = enc_len > GOVEE_H5059_LEAK_FLAG2_OFFSET ? dec[GOVEE_H5059_LEAK_FLAG2_OFFSET] : -1;
    int leak_status = GOVEE_H5059_LEAK_UNKNOWN;

    char const *event = "Unknown";
    if (msg_class == GOVEE_H5059_MSG_CLASS_TELEMETRY) {
        event = "Telemetry";
        if (subtype == GOVEE_H5059_SUBTYPE_BUTTON) {
            event = "Button Press";
            leak_status = GOVEE_H5059_LEAK_DRY;
        }
        else if (subtype == GOVEE_H5059_SUBTYPE_LEAK && leak_flag_1 == GOVEE_H5059_LEAK_FLAG_ACTIVE && leak_flag_2 == GOVEE_H5059_LEAK_FLAG_ACTIVE) {
            event = "Water Leak";
            leak_status = GOVEE_H5059_LEAK_WET;
        }
        else if (subtype == GOVEE_H5059_SUBTYPE_POST_ALARM) {
            event = "Post Alarm";
        }
    }
    else if (msg_class == GOVEE_H5059_MSG_CLASS_PAIRING) {
        event = "Pairing";
    }
    else if (msg_class == GOVEE_H5059_MSG_CLASS_OTHER) {
        event = "Class 0x02";
    }

    /* clang-format off */
    data_t *data = data_make(
            "model",        "",                 DATA_STRING, "Govee-H5059",
            "id",           "",                 DATA_FORMAT, "%08x", DATA_INT, id,
            "id_wire",      "",                 DATA_FORMAT, "%08x", DATA_INT, id_wire,
            "event",        "",                 DATA_STRING, event,
            "msg_class",    "",                 DATA_FORMAT, "0x%02x", DATA_INT, msg_class,
            "subtype",      "",                 DATA_COND,   subtype >= 0, DATA_FORMAT, "0x%02x", DATA_INT, subtype,
            "detect_wet",   "",                 DATA_COND,   leak_status >= 0, DATA_INT, leak_status,
            "mic",          "Integrity",        DATA_STRING, "CRC",
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "id",
        "id_wire",
        "event",
        "msg_class",
        "subtype",
        "detect_wet",
        "mic",
        NULL,
};

r_device const govee_h5059 = {
        .name        = "Govee Water Leak Detector H5059",
        .modulation  = FSK_PULSE_PCM,
        .short_width = 100,
        .long_width  = 100,
        .reset_limit = 2000,
        .decode_fn   = &govee_h5059_decode,
        .fields      = output_fields,
};
