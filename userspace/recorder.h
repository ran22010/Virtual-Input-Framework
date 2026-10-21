#ifndef VINPUT_RECORDER_H
#define VINPUT_RECORDER_H

#include <signal.h>
#include <stddef.h>
#include "macro.h"

/* Captures for duration_seconds, or until *stop becomes nonzero.
 * Returns a heap-owned array in *events_out (caller must free it).
 * Events are normalized to timestamps starting at zero and virtual IDs.
 * Return value: 0 or negative errno.
 */
int vinput_record(const char *device_path, unsigned int duration_seconds,
                  size_t max_events, const volatile sig_atomic_t *stop,
                  struct vinput_event **events_out, size_t *count_out);

#endif
