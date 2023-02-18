#ifndef __DEVICE_H_
#define __DEVICE_H_

#include <common.h>
size_t dispinfo_read(void *buffer, size_t offset, size_t length);
size_t fb_write(const void *buffer, size_t offset, size_t length);
size_t events_read(void *buffer, size_t offset, size_t length);
size_t serial_write(const void *buffer, size_t offset, size_t length);

void init_device();

#endif