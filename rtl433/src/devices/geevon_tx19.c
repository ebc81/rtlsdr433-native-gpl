/** @file
    Geevon TX19-1 Remote Outdoor Sensor with LCD Display.

    Contributed by Matt Falcon <falcon4@gmail.com>
    Copyright (C) 2026 Christian W. Zuckschwerdt <zany@triq.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "decoder.h"

/**
Geevon TX19-1 Remote Outdoor Sensor with LCD Display.
*/

static int geevon_tx19_decode(r_device *decoder, bitbuffer_t *bitbuffer)
{
    bitbuffer_invert(bitbuffer);

    int r = bitbuffer_find_repeated_prefix(bitbuffer, bitbuffer->num_rows > 5 ? 5 : 3, 72);
    if (r < 0) {
        return DECODE_ABORT_LENGTH;
    }

    uint8_t *b = bitbuffer->bb[r];

    if (bitbuffer->bits_per_row[r] != 73) {
        decoder_log(decoder, 1, __func__, "Bit length did NOT match.");
        return DECODE_ABORT_LENGTH;
    }

    if (b[5] != 0xAA || b[6] != 0x55 || b[7] != 0xAA) {
        decoder_log(decoder, 1, __func__, "Fixed bytes did NOT match.");
        return DECODE_FAIL_MIC;
    }

    uint8_t chk = lfsr_digest8_reverse(b, 8, 0x98, 0x25);
    if (chk != b[8]) {
        decoder_log(decoder, 1, __func__, "Checksum did NOT match.");
        return DECODE_FAIL_MIC;
    }

    int battery_low = (b[1] >> 7);
    int channel     = ((b[1] & 0x30) >> 4) + 1;
    int temp_raw    = (b[2] << 4) | (b[3] >> 4);
    float temp_c    = (temp_raw - 500) * 0.1f;
    int humidity    = b[4];

    /* clang-format off */
    data_t *data = data_make(
            "model",            "",             DATA_STRING, "Geevon-TX191",
            "id",               "",             DATA_INT,    b[0],
            "battery_ok",       "Battery",      DATA_INT,    !battery_low,
            "channel",          "Channel",      DATA_INT,    channel,
            "temperature_C",    "Temperature",  DATA_FORMAT, "%.1f C", DATA_DOUBLE, temp_c,
            "humidity",         "Humidity",     DATA_FORMAT, "%u %%", DATA_INT,     humidity,
            "mic",              "Integrity",    DATA_STRING, "CRC",
            NULL);
    /* clang-format on */

    decoder_output_data(decoder, data);
    return 1;
}

static char const *const output_fields[] = {
        "model",
        "battery",
        "channel",
        "temperature_C",
        "humidity",
        "mic",
        NULL,
};

r_device const geevon_tx19 = {
        .name        = "Geevon TX19-1 outdoor sensor",
        .modulation  = OOK_PULSE_PWM,
        .short_width = 250,
        .long_width  = 500,
        .sync_width  = 750,
        .gap_limit   = 625,
        .reset_limit = 1700,
        .decode_fn   = &geevon_tx19_decode,
        .fields      = output_fields,
};
