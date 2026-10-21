#ifndef VINPUT_MACRO_FILE_H
#define VINPUT_MACRO_FILE_H

#include <stddef.h>
#include "macro.h"

/* File format v1: 24-byte native-endian header, followed by consecutive
 * native-ABI vinput_event records. Intended for the same Linux architecture.
 * Return 0 on success, or negative errno.
 */
int macro_file_save(const char *path, const struct vinput_event *events,
                    size_t count);
int macro_file_load(const char *path, struct vinput_event **events_out,
                    size_t *count_out);

#endif
