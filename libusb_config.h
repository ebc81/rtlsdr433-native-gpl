#ifndef LIBUSB_CONFIG_H
#define LIBUSB_CONFIG_H

#define HAVE_GETTIMEOFDAY 1
#define HAVE_POLL_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_UNISTD_H 1
#define POLL_NFDS_TYPE nfds_t
#define THREADS_POSIX 1
#define OS_LINUX 1
#define _GNU_SOURCE 1

#define DEFAULT_VISIBILITY __attribute__((visibility("default")))

#endif
