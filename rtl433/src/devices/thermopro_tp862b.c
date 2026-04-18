/** @file
    ThermoPro TP862b TempSpike XR 1,000-ft Wireless Dual-Probe Meat Thermometer.

    Copyright (C) 2026 n6ham <github.com/n6ham>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "decoder.h"

/**
ThermoPro TP862b TempSpike XR Wireless Dual-Probe Meat Thermometer.
*/

static int thermopro_tp862b_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    uint8_t const preamble_pattern[] = {0xd2, 0x55, 0x2d, 0xd4};
    uint8_t b[9];

    if (bitbuffer->num_rows > 1) {
        return DECODE_FAIL_SANITY;
    }
    int msg_len = bitbuffer->bits_per_row[0];
    if (msg_len != 170) {
        return DECODE_ABORT_LENGTH;
    }

    int offset = bitbuffer_search(bitbuffer, 0, 0, preamble_pattern, sizeof(preamble_pattern) * 8);
    if (offset >= msg_len) {
        return DECODE_ABORT_EARLY;
    }

    offset += sizeof(preamble_pattern) * 8;
    bitbuffer_extract_bytes(bitbuffer, 0, offset, b, 9 * 8);

    if (b[7] == ~b[8]) {
        return DECODE_FAIL_MIC;
    }

    uint8_t calc_crc = crc8(b, 7, 0x07, 0x00) ^ 0xdb;
    if (calc_crc != b[7]) {
        return DECODE_FAIL_MIC;
    }

    uint8_t id            = b[0];
    uint8_t probe         = b[1];
    uint16_t internal_raw = (b[2] << 4) | (b[3] >> 4);
    uint16_t ambient_raw  = ((b[3] & 0x0f) << 8) | b[4];
    uint8_t flags         = b[5];

    float internal_c = (internal_raw - 500) * 0.1f;
    float ambient_c  = (ambient_raw - 500) * 0.1f;

    /* clang-format off */
    data_t *data = data_make(
            "model",             "",             DATA_STRING, "ThermoPro-TP862b",
            "id",                "",             DATA_FORMAT, "%02x",   DATA_INT,    id,
            "color",             "Color",        DATA_STRING, (probe & 0x10) ? "white" : "black",
            "is_docked",         "Docked",       DATA_INT,    (probe & 0x40) >> 6,
            "temperature_int_C", "Internal",     DATA_FORMAT, "%.1f C", DATA_DOUBLE, internal_c,
            "temperature_amb_C", "Ambient",      DATA_FORMAT, "%.1f C", DATA_DOUBLE, ambient_c,
            "flags",             "Flags",        DATA_FORMAT, "%02x",   DATA_INT,    flags,
            "mic",               "Integrity",    DATA_STRING, "CRC",
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "id",
        "color",
        "is_docked",
        "temperature_int_C",
        "temperature_amb_C",
        "flags",
        "mic",
        NULL,
};

r_device const thermopro_tp862b = {
        .name        = "ThermoPro TP862b TempSpike XR Wireless Dual-Probe Meat Thermometer",
        .modulation  = FSK_PULSE_PCM,
        .short_width = 104,
        .long_width  = 104,
        .reset_limit = 2000,
        .decode_fn   = &thermopro_tp862b_decode,
        .fields      = output_fields,
};
