/*
 * rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
 *
 * Modification 2013 by Martin Marinov <martintzvetomirov@gmail.com>
 * Modifications: opening a device via file descriptor
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __RTL_ANDRO_SDR_H
#define __RTL_ANDRO_SDR_H

#include "rtl-sdr/include/rtl-sdr.h"

#ifdef __cplusplus
extern "C" {
#endif

RTLSDR_API int rtlsdr_open2(rtlsdr_dev_t **out_dev, int fd);
RTLSDR_API int rtlsdr_cancel_async_save(rtlsdr_dev_t *dev);
RTLSDR_API int rtlsdr_cancel_async_save_fast(rtlsdr_dev_t *dev);
RTLSDR_API int rtlsdr_supporting_ppm_search();

#ifdef __cplusplus
}
#endif

#endif /* __RTL_ANDRO_SDR_H */
