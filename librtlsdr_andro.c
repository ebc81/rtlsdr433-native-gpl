/*
 * librtlsdr_wrapper is na addition to the original librtlsdr
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
 *
 * Modification 2016 by Christian Ebner <cebner@gmx.at>
 * rtlsdr_open2 base on librtlsdr_andro from Martin Marinov <martintzvetomirov@gmail.com> 2012
 */

#include "librtlsdr_andro.h"
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libusb.h>

// We include the actual source to access private structures if needed,
// but since we are compiling everything, it's better to just link.
// However, the reference project uses this #include approach.
#include "rtl-sdr/src/librtlsdr.c"

#define aprintf_stderr(...) __android_log_print(ANDROID_LOG_ERROR, "LIBRTLSDR", __VA_ARGS__)

#define EXIT_TEST_RTLSDR_OPEN1 -101
#define EXIT_TEST_RTLSDR_OPEN2 -102

int rtlsdr_open2(rtlsdr_dev_t **out_dev, int fd)
{
    int r;
    rtlsdr_dev_t *dev = NULL;
    uint8_t reg;
    uint8_t buf[EEPROM_SIZE];
    int pos;

    __android_log_print(ANDROID_LOG_INFO, "RTL433_USB",
            "rtlsdr_open2: opening device with fd=%d", fd);

    dev = malloc(sizeof(rtlsdr_dev_t));
    if (NULL == dev)
        return -ENOMEM;

    memset(dev, 0, sizeof(rtlsdr_dev_t));
    memcpy(dev->fir, fir_default, sizeof(fir_default));

    int status = libusb_init(&dev->ctx);
    if (status != LIBUSB_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, "RTL433_USB",
                "rtlsdr_open2: libusb_init failed with status=%d", status);
        free(dev);
        return status;
    }
    else if (dev->ctx == NULL)
    {
        __android_log_write(ANDROID_LOG_ERROR, "RTL433_USB",
                "rtlsdr_open2: libusb_init returned SUCCESS but ctx is NULL");
        free(dev);
        return EXIT_TEST_RTLSDR_OPEN1;
    }
    __android_log_write(ANDROID_LOG_INFO, "RTL433_USB", "rtlsdr_open2: libusb_init OK");

    dev->dev_lost = 1;

    status = libusb_wrap_sys_device(dev->ctx, fd, &dev->devh);
    if (status != LIBUSB_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, "RTL433_USB",
                "rtlsdr_open2: libusb_wrap_sys_device failed with status=%d", status);
        libusb_exit(dev->ctx);
        free(dev);
        return status;
    }
    __android_log_write(ANDROID_LOG_INFO, "RTL433_USB",
            "rtlsdr_open2: libusb_wrap_sys_device OK");

    r = libusb_claim_interface(dev->devh, 0);
    if (r < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "RTL433_USB",
                "rtlsdr_open2: libusb_claim_interface error %d", r);
        libusb_close(dev->devh);
        libusb_exit(dev->ctx);
        free(dev);
        return EXIT_TEST_RTLSDR_OPEN2;
    }
    __android_log_write(ANDROID_LOG_INFO, "RTL433_USB",
            "rtlsdr_open2: interface claimed OK");

    dev->rtl_xtal = DEF_RTL_XTAL_FREQ;

    /* perform a dummy write, if it fails, reset the device */
    if (rtlsdr_write_reg(dev, USBB, USB_SYSCTL, 0x09, 1) < 0) {
        aprintf_stderr("Resetting device...\n");
        libusb_reset_device(dev->devh);
    }

    rtlsdr_init_baseband(dev);
    dev->dev_lost = 0;

    r = rtlsdr_read_eeprom(dev, buf, 0, EEPROM_SIZE);
    pos = get_string_descriptor(STR_OFFSET, buf, dev->manufact);
    get_string_descriptor(pos, buf, dev->product);

    /* Probe tuners */
    rtlsdr_set_i2c_repeater(dev, 1);

    reg = rtlsdr_i2c_read_reg(dev, E4K_I2C_ADDR, E4K_CHECK_ADDR);
    if (reg == E4K_CHECK_VAL) {
        __android_log_write(ANDROID_LOG_INFO, "RTL433_USB", "Tuner detected: Elonics E4000");
        dev->tuner_type = RTLSDR_TUNER_E4000;
        goto found;
    }

    reg = rtlsdr_i2c_read_reg(dev, FC0013_I2C_ADDR, FC0013_CHECK_ADDR);
    if (reg == FC0013_CHECK_VAL) {
        __android_log_write(ANDROID_LOG_INFO, "RTL433_USB", "Tuner detected: Fitipower FC0013");
        dev->tuner_type = RTLSDR_TUNER_FC0013;
        goto found;
    }

    reg = rtlsdr_i2c_read_reg(dev, R820T_I2C_ADDR, R82XX_CHECK_ADDR);
    if (reg == R82XX_CHECK_VAL) {
        __android_log_write(ANDROID_LOG_INFO, "RTL433_USB", "Tuner detected: Rafael Micro R820T");
        dev->tuner_type = RTLSDR_TUNER_R820T;
        goto found;
    }

    reg = rtlsdr_i2c_read_reg(dev, R828D_I2C_ADDR, R82XX_CHECK_ADDR);
    if (reg == R82XX_CHECK_VAL) {
        __android_log_write(ANDROID_LOG_INFO, "RTL433_USB", "Tuner detected: Rafael Micro R828D");
        if (rtlsdr_check_dongle_model(dev, "RTLSDRBlog", "Blog V4"))
            __android_log_write(ANDROID_LOG_INFO, "RTL433_USB", "RTL-SDR Blog V4 variant");
        dev->tuner_type = RTLSDR_TUNER_R828D;
        goto found;
    }

    reg = rtlsdr_i2c_read_reg(dev, FC2580_I2C_ADDR, FC2580_CHECK_ADDR);
    if ((reg & 0x7f) == FC2580_CHECK_VAL) {
        __android_log_write(ANDROID_LOG_INFO, "RTL433_USB", "Tuner detected: FCI 2580");
        dev->tuner_type = RTLSDR_TUNER_FC2580;
        goto found;
    }

    reg = rtlsdr_i2c_read_reg(dev, FC0012_I2C_ADDR, FC0012_CHECK_ADDR);
    if (reg == FC0012_CHECK_VAL) {
        __android_log_write(ANDROID_LOG_INFO, "RTL433_USB", "Tuner detected: Fitipower FC0012");
        dev->tuner_type = RTLSDR_TUNER_FC0012;
        goto found;
    }

found:
    dev->tun_xtal = dev->rtl_xtal;
    dev->tuner = &tuners[dev->tuner_type];

    switch (dev->tuner_type) {
        case RTLSDR_TUNER_R828D:
            /* If NOT an RTL-SDR Blog V4, set typical R828D 16 MHz freq. Otherwise, keep at 28.8 MHz. */
            if (!(rtlsdr_check_dongle_model(dev, "RTLSDRBlog", "Blog V4"))) {
                dev->tun_xtal = R828D_XTAL_FREQ;
            }
            /* fall-through */
        case RTLSDR_TUNER_R820T:
            /* disable Zero-IF mode */
            rtlsdr_demod_write_reg(dev, 1, 0xb1, 0x1a, 1);

            /* only enable In-phase ADC input */
            rtlsdr_demod_write_reg(dev, 0, 0x08, 0x4d, 1);

            /* the R82XX use 3.57 MHz IF for the DVB-T 6 MHz mode, and
             * 4.57 MHz for the 8 MHz mode */
            rtlsdr_set_if_freq(dev, R82XX_IF_FREQ);

            /* enable spectrum inversion */
            rtlsdr_demod_write_reg(dev, 1, 0x15, 0x01, 1);
            break;
        case RTLSDR_TUNER_UNKNOWN:
            aprintf_stderr("No supported tuner found\n");
            rtlsdr_set_direct_sampling(dev, 1);
            break;
        default:
            break;
    }
    /* Hack to force the Bias T to always be on if we set the IR-Endpoint
    * bit in the EEPROM to 0. Default on EEPROM is 1.
    */
    dev->force_bt = (buf[7] & 0x02) ? 0 : 1;
    if(dev->force_bt)
        rtlsdr_set_bias_tee(dev, 1);

    if (dev->tuner->init) {
        r = dev->tuner->init(dev);
    }

    rtlsdr_set_i2c_repeater(dev, 0);
    *out_dev = dev;

    __android_log_print(ANDROID_LOG_INFO, "RTL433_USB",
            "rtlsdr_open2: SUCCESS — tuner_type=%d force_bt=%d",
            dev->tuner_type, dev->force_bt);
    return 0;
}

int rtlsdr_cancel_async_save_fast(rtlsdr_dev_t *dev)
{
    if (!dev) return -1;
    if (dev->async_status == RTLSDR_RUNNING && dev->dev_lost == 0)
    {
        return rtlsdr_cancel_async(dev);
    }
    return 0;
}

int rtlsdr_cancel_async_save(rtlsdr_dev_t *dev)
{
    if (!dev) return -1;
    if (dev->async_status == RTLSDR_RUNNING && dev->dev_lost == 0)
    {
        int r = rtlsdr_cancel_async(dev);
        int timeout = 0;
        while (timeout < 1000) {
            usleep(1000);
            timeout++;
            if (dev->async_status == RTLSDR_INACTIVE || dev->dev_lost)
                break;
        }
        return r;
    }
    return 0;
}


