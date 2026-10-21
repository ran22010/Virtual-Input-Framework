#ifndef VINPUT_TEST_COMMON_H
#define VINPUT_TEST_COMMON_H

#include <linux/input.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "uapi/vinput_uapi.h"

#define VIF_PATH "/dev/vif_driver"
#define TEST_TIMEOUT_MS 5000

long long now_ms(void);
int open_vif(void);
struct vinput_event make_vinput_event(unsigned int source_id,
                                      unsigned int type,
                                      unsigned int code, int value);
int check_errno(const char *description, ssize_t result, int expected_errno);
int check_count(const char *description, ssize_t result, size_t expected);
int find_evdev_by_name(const char *name, char *out, size_t out_size);
int drain_fd(int fd, size_t record_size, int max_batches);

#endif
