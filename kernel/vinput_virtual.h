#ifndef VINPUT_VIRTUAL_H
#define VINPUT_VIRTUAL_H

#include <linux/input.h>

#include "uapi/vinput_uapi.h"
#include "vinput_ringbuf.h"

#define VINPUT_VIRTUAL_KEYBOARD 0
#define VINPUT_VIRTUAL_MOUSE    1
#define VINPUT_VIRTUAL_DEVICES  2

int vinput_virtual_init(struct vinput_ringbuf *output_rb);

void vinput_virtual_exit(void);

#endif