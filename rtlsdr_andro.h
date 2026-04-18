#ifndef RTLSDR_ANDRO_H
#define RTLSDR_ANDRO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// We need a way to define rtlsdr_dev_t if we don't have the full headers yet
typedef struct rtlsdr_dev rtlsdr_dev_t;

int rtlsdr_open2(rtlsdr_dev_t **out_dev, int fd);
int rtlsdr_cancel_async_save(rtlsdr_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif
