# rtlsdr433-native-gpl

**GPL-2.0 Native Layer — RTL-SDR 433 Android App**

This repository contains the native C/C++ signal-processing layer for the
[RTL-SDR 433](https://play.google.com/store/apps/details?id=eu.ebctech.rtlsdr433andro)
Android application.

It is published here to satisfy the **GPL-2.0 written offer requirement**.
The RTL-SDR 433 app binary incorporates GPL-2.0 licensed code from
[rtl_433](https://github.com/merbanan/rtl_433) and
[rtl-sdr](https://osmocom.org/projects/rtl-sdr). As required by GPL §3,
the corresponding source code is made available here.

---

## What's in this repository

| Path | Description | License |
|---|---|---|
| `rtl433/` | rtl_433 v25.12 (patched for Android, `__EBCANDROID__`) | GPL-2.0 |
| `rtl-sdr/` | rtl-sdr library (patched for Android) | GPL-2.0 |
| `libusb-andro/` | libusb 1.0.23 Android port | LGPL-2.1 |
| `android_bridge.c` | EBC custom `data_output_t` → JSON → JNI callback | GPL-2.0 |
| `rtlsdr433.cpp` | JNI entry points and C→Kotlin callbacks | GPL-2.0 |
| `librtlsdr_andro.c` | Android USB open via `libusb_wrap_sys_device(fd)` | GPL-2.0 |
| `CMakeLists.txt` | Android NDK build configuration | GPL-2.0 |

The proprietary Kotlin application layer (UI, billing, service logic) is **not**
included here and is not covered by GPL-2.0.

---

## Android Patches (`__EBCANDROID__`)

All Android-specific patches in upstream rtl_433/rtl-sdr source files are guarded
by `#ifdef __EBCANDROID__`. Key patches:

- **`rtl433/src/rtl_433.c`** — `rtl433_start()` / `rtl433_android_close()` entry block;
  `acquire_callback` bypass of mongoose (`sdr_callback()` called directly)
- **`rtl433/src/sdr.c`** (sdr.c in root) — `sdr_open_rtl()` opens via USB fd using `rtlsdr_open2()`
- **`rtl433/include/rtl_433.h`** — extern declarations + Android API prototypes
- **`rtl433/src/output_rtltcp.c`** — `pthread_cancel` guarded for Bionic

---

## Building

This directory is consumed by the Android NDK build system. It is not intended
to build standalone. To build the full app, see the private repository
[ebc81/rtlsdr433_android](https://github.com/ebc81/rtlsdr433_android).

To build just the native library for inspection purposes, you need:
- Android NDK ≥ 29.0.14206865
- CMake ≥ 4.1.2
- Include this directory as the `externalNativeBuild.cmake.path` in an
  Android module's `build.gradle`

---

## Version History

| Tag | App Version | Date |
|---|---|---|
| `v1.0.9` | 1.0.9 | 2026-04-18 |

---

## License

The files in this repository are derived from or link with GPL-2.0 licensed
software and are distributed under the **GNU General Public License version 2.0**.

```
Copyright (C) 2025 Christian Ebner <info@ebctech.eu>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
```

Full GPL-2.0 text: `rtl433/COPYING` · Full LGPL-2.1 text: `libusb-andro/COPYING`

---

## Contact

ebctech.eu · [info@ebctech.eu](mailto:info@ebctech.eu) · [https://www.ebctech.eu](https://www.ebctech.eu)

For the RTL-SDR 433 app: [Google Play Store](https://play.google.com/store/apps/details?id=eu.ebctech.rtlsdr433andro)
