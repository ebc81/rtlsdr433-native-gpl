# rtlsdr433-native-gpl

**GPL-2.0 Native Layer — RTL-SDR 433 Android App**

This repository contains the native C/C++ signal-processing layer for the
[RTL-SDR 433](https://play.google.com/store/apps/details?id=eu.ebctech.rtlsdr433andro)
Android application.

It is published here to satisfy the **GPL-2.0 written offer requirement**.
The RTL-SDR 433 app binary incorporates GPL-2.0 licensed code from
[rtl_433](https://github.com/merbanan/rtl_433), and the corresponding source
code for it is made available here as required by GPL §3.

Since app version **v1.3.3** the SDR driver layer — `rtl-sdr`, the libusb
Android port and the USB file-descriptor bridge — is **no longer in this
repository**. It is a separate shared project with its own source route:

> **[github.com/ebc81/ebc-sdr-native](https://github.com/ebc81/ebc-sdr-native)**,
> pinned by app v1.3.3 at tag **`v0.3.0`**

It moved out because three EBC SDR applications share that code, and keeping a
copy per app meant fixes flowed in one direction only. Republishing it here as
well would put the same GPL code in two places under two different versions,
which is exactly the drift the shared repository exists to end. The app's
`NOTICE` file names both routes.

---

## What's in this repository

| Path | Description | License |
|---|---|---|
| `rtl433/` | rtl_433 v25.12 (patched for Android, `__EBCANDROID__`) | GPL-2.0 |
| `android_bridge.c` | EBC custom `data_output_t` → JSON → JNI callback | GPL-2.0 |
| `rtlsdr433.cpp` | JNI entry points and C→Kotlin callbacks | GPL-2.0 |
| `CMakeLists.txt` | Android NDK build configuration | GPL-2.0 |

### Where the rest lives

| Component | License | Source |
|---|---|---|
| `rtl-sdr` (patched for Android) | GPL-2.0 | [ebc-sdr-native](https://github.com/ebc81/ebc-sdr-native) @ `v0.3.0` |
| libusb 1.0.23 Android port | LGPL-2.1 | [ebc-sdr-native](https://github.com/ebc81/ebc-sdr-native) @ `v0.3.0` |
| `librtlsdr_andro.c` — USB open via `libusb_wrap_sys_device(fd)` | GPL-2.0 | [ebc-sdr-native](https://github.com/ebc81/ebc-sdr-native) @ `v0.3.0` |

In the app's build tree these are a git submodule at
`app/src/main/cpp/ebc-sdr-native`, which is why `CMakeLists.txt` here refers to
`add_subdirectory(ebc-sdr-native)` and links a target named `ebc_sdr` that is
not part of this repository.

The proprietary Kotlin application layer (UI, billing, service logic) is **not**
included here and is not covered by GPL-2.0.

---

## Android Patches (`__EBCANDROID__`)

All Android-specific patches in upstream rtl_433 source files are guarded
by `#ifdef __EBCANDROID__`. Key patches:

- **`rtl433/src/rtl_433.c`** — `rtl433_start()` / `rtl433_android_close()` entry block;
  `acquire_callback` bypass of mongoose (`sdr_callback()` called directly)
- **`rtl433/src/sdr.c`** — `sdr_open_rtl()` opens via the Android USB file descriptor
  using `rtlsdr_open2()`, whose prototype comes from `librtlsdr_andro.h` in
  `ebc-sdr-native`
- **`rtl433/include/rtl_433.h`** — extern declarations + Android API prototypes
- **`rtl433/src/output_rtltcp.c`** — `pthread_cancel` guarded for Bionic

---

## Building

This directory is consumed by the Android NDK build system. It is not intended
to build standalone, and it is **not self-contained**: `CMakeLists.txt` expects
`ebc-sdr-native` to be present as a subdirectory (see above).

To build just the native library for inspection purposes, you need:
- Android NDK ≥ 29.0.14206865
- CMake ≥ 4.1.2
- A checkout of [ebc-sdr-native](https://github.com/ebc81/ebc-sdr-native) at the
  tag named above, placed in a directory called `ebc-sdr-native` here
- Include this directory as the `externalNativeBuild.cmake.path` in an
  Android module's `build.gradle`

---

## Version History

Tags mark the native content that shipped with a given app version. A release
that changed nothing under the native layer is tagged on the same commit as the
release before it — so several tags may point at one commit.

| Tag | App version | Native content synced from | Date |
|---|---|---|---|
| `v1.3.3` | 1.3.3 | 1.3.3 — rtl-sdr and libusb moved to `ebc-sdr-native` | 2026-09-04 |
| `v1.3.1` | 1.3.1 | 1.2.9 (no native change in 1.3.0 / 1.3.1) | 2026-05-31 |
| `v1.3.0` | 1.3.0 | 1.2.9 (no native change) | 2026-05-31 |
| `v1.2.9` | 1.2.9 | 1.2.9 | 2026-05-31 |
| `v1.2.8` | 1.2.8 | 1.2.8 | 2026-05-15 |
| `v1.2.7` | 1.2.7 | 1.2.6 — tag name and synced version differ | 2026-05-10 |
| `v1.0.17` | 1.0.17 | 1.0.17 | 2026-04-23 |
| `v1.0.10` | 1.0.10 | 1.0.10 | 2026-04-18 |
| `v1.0.9` | 1.0.9 | 1.0.9 (initial commit) | 2026-04-18 |

Two known gaps, left as they are because moving a published tag would break a
GPL source pointer somebody may already have followed:

- There is **no `v1.3.2` tag**. App v1.3.2 changed nothing in the native layer;
  its native content is the one tagged `v1.3.1`.
- **`v1.2.7`** sits on the commit that synced app v1.2.6.

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

Full GPL-2.0 text: `rtl433/COPYING` · Full LGPL-2.1 text: in
[ebc-sdr-native](https://github.com/ebc81/ebc-sdr-native), at
`libusb-andro/COPYING`

---

## Contact

ebctech.eu · [info@ebctech.eu](mailto:info@ebctech.eu) · [https://www.ebctech.eu](https://www.ebctech.eu)

For the RTL-SDR 433 app: [Google Play Store](https://play.google.com/store/apps/details?id=eu.ebctech.rtlsdr433andro)
