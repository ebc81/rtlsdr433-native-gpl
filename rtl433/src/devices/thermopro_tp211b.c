/** @file
    ThermoPro TP211B Thermometer.

    Copyright (C) 2026, Ali Rahimi, Bruno OCTAU, Christian W. Zuckschwerdt

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "decoder.h"

/**
ThermoPro TP211B Thermometer.
S.a. issue #3435
*/

static uint16_t tp211b_checksum(uint8_t const *b)
{
    static uint16_t const xor_table[] = {
            0xC881, 0xC441, 0xC221, 0xC111, 0xC089, 0xC045, 0xC023, 0xC010,
            0xC01F, 0xC00E, 0x6007, 0x9002, 0x4801, 0x8401, 0xE201, 0xD101,
            0xDE01, 0xCF01, 0xC781, 0xC3C1, 0xC1E1, 0xC0F1, 0xC079, 0xC03D,
            0xC029, 0xC015, 0xC00B, 0xC004, 0x6002, 0x3001, 0xB801, 0xFC01,
            0xE801, 0xD401, 0xCA01, 0xC501, 0xC281, 0xC141, 0xC0A1, 0xC051,
            0xC061, 0xC031, 0xC019, 0xC00D, 0xC007, 0xC002, 0x6001, 0x9001};
    uint16_t checksum = 0x411b;
    for (int n = 0; n < 6; n++) {
        for (int i = 0; i < 8; i++) {
            const int bit = (b[n] << (i + 1)) & 0x100;
            if (bit) {
                checksum ^= xor_table[(n * 8) + i];
            }
        }
    }
    return checksum;
}

static int thermopro_tp211b_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    uint8_t const preamble_pattern[] = {0x55, 0x2d, 0xd4};
    uint8_t b[8];

    if (bitbuffer->num_rows > 1) {
        return DECODE_FAIL_SANITY;
    }
    const int msg_len = bitbuffer->bits_per_row[0];

    int offset = bitbuffer_search(bitbuffer, 0, 0, preamble_pattern, sizeof(preamble_pattern) * 8);
    if (offset >= msg_len) {
        return DECODE_ABORT_EARLY;
    }

    if ((msg_len - offset) < 64) {
        return DECODE_ABORT_LENGTH;
    }

    offset += sizeof(preamble_pattern) * 8;
    bitbuffer_extract_bytes(bitbuffer, 0, offset, b, 8 * 8);

    if (b[5] != 0xaa) {
        return DECODE_FAIL_SANITY;
    }

    if ((!b[0] && !b[1] && !b[2] && !b[3] && !b[4]) || (b[0] == 0xff && b[1] == 0xff && b[2] == 0xff && b[3] == 0xff && b[4] == 0xff)) {
        return DECODE_FAIL_SANITY;
    }

    const uint16_t checksum_calc     = tp211b_checksum(b);
    const uint16_t checksum_from_row = b[6] << 8 | b[7];
    if (checksum_from_row != checksum_calc) {
        return DECODE_FAIL_MIC;
    }

    int id       = (b[0] << 16) | (b[1] << 8) | b[2];
    int temp_raw = ((b[3] & 0x0f) << 8) | b[4];
    float temp_c = (temp_raw - 500) * 0.1f;

    /* clang-format off */
    data_t *data = data_make(
            "model",         "",            DATA_STRING, "ThermoPro-TP211B",
            "id",            "Id",          DATA_FORMAT, "%06x",   DATA_INT,    id,
            "temperature_C", "Temperature", DATA_FORMAT, "%.1f C", DATA_DOUBLE, (double)temp_c,
            "mic",           "Integrity",   DATA_STRING, "CHECKSUM",
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "id",
        "temperature_C",
        "mic",
        NULL,
};

r_device const thermopro_tp211b = {
        .name        = "ThermoPro TP211B Thermometer",
        .modulation  = FSK_PULSE_PCM,
        .short_width = 105,
        .long_width  = 105,
        .reset_limit = 1500,
        .decode_fn   = &thermopro_tp211b_decode,
        .fields      = output_fields,
};
