/** @file
    Sainlogic SA8 Weather Station.

    Copyright (C) 2026 Bruno OCTAU (\@ProfBoc75)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "decoder.h"

/**
Sainlogic SA8, Gevanti SA8 Weather Station.
S.a. issue #3445
*/

static int sainlogic_sa8_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    uint8_t const preamble_pattern[] = {0xfc, 0x95};
    uint8_t b[41];

    if (bitbuffer->num_rows != 1) {
        return DECODE_ABORT_EARLY;
    }

    unsigned offset = bitbuffer_search(bitbuffer, 0, 0, preamble_pattern, 16) + 16;

    if (offset >= bitbuffer->bits_per_row[0]) {
        return DECODE_ABORT_EARLY;
    }

    int num_bits = bitbuffer->bits_per_row[0] - offset;
    num_bits     = MIN((size_t)num_bits, sizeof(b) * 10);
    int len      = extract_bytes_uart(bitbuffer->bb[0], offset, num_bits, b);

    if (len < 41) {
        return DECODE_ABORT_LENGTH;
    }

    uint16_t crc_calculated = crc16(&b[3], 36, 0x8005, 0xffff);
    if (crc_calculated != (b[40] << 8 | b[39])) {
        return DECODE_FAIL_MIC;
    }

    char ID[13];
    snprintf(ID, sizeof(ID), "%02x%02x%02x%02x%02x%02x", b[4], b[3], b[6], b[5], b[8], b[7]);
    uint16_t counter  = b[16] << 8 | b[15];
    int16_t temp_raw  = b[20] << 8 | b[19];
    int humidity      = b[21];
    int gust_raw      = b[28] << 8 | b[27];
    int wind_raw      = b[30] << 8 | b[29];
    int dir_degree    = b[32] << 8 | b[31];
    uint16_t rain_raw = b[34] << 8 | b[33];
    uint16_t unknown  = b[36] << 8 | b[35];
    int battery_ok    = (b[38] & 0x10) >> 4;
    uint16_t bat_mv   = b[38] << 8 | b[37];

    float temp_c    = temp_raw * 0.1f;
    float gust_km_h = gust_raw * 0.036f;
    float wind_km_h = wind_raw * 0.036f;
    float rain_mm   = rain_raw * 0.42893617f;

    /* clang-format off */
    data_t *data = data_make(
            "model",         "",               DATA_STRING, "Sainlogic-SA8",
            "id",            "",               DATA_STRING, ID,
            "battery_ok",    "Battery_OK",     DATA_INT,    battery_ok,
            "counter",       "Counter",        DATA_INT,    counter,
            "temperature_C", "Temperature",    DATA_FORMAT, "%.1f C",    DATA_DOUBLE, temp_c,
            "humidity",      "Humidity",       DATA_FORMAT, "%u %%",     DATA_INT,    humidity,
            "wind_avg_km_h", "Wind avg speed", DATA_FORMAT, "%.1f km/h", DATA_DOUBLE, wind_km_h,
            "wind_max_km_h", "Wind max speed", DATA_FORMAT, "%.1f km/h", DATA_DOUBLE, gust_km_h,
            "wind_dir_deg",  "Wind Direction", DATA_INT,    dir_degree,
            "rain_mm",       "Total rainfall", DATA_FORMAT, "%.1f mm",   DATA_DOUBLE, rain_mm,
            "unknown",       "Unknown",        DATA_FORMAT, "%04x",      DATA_INT,    unknown,
            "flags",         "Flags",          DATA_FORMAT, "%04x",      DATA_INT,    bat_mv,
            "mic",           "Integrity",      DATA_STRING, "CRC",
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "id",
        "battery_ok",
        "counter",
        "temperature_C",
        "humidity",
        "wind_avg_km_h",
        "wind_max_km_h",
        "wind_dir_deg",
        "rain_mm",
        "unknown",
        "flags",
        "mic",
        NULL,
};

r_device const sainlogic_sa8 = {
        .name        = "Sainlogic SA8, Gevanti SA8 Weather Station",
        .modulation  = OOK_PULSE_PCM,
        .short_width = 200,
        .long_width  = 200,
        .reset_limit = 2500,
        .decode_fn   = &sainlogic_sa8_decode,
        .fields      = output_fields,
};
