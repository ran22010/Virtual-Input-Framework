#ifndef VINPUT_PLAYER_H
#define VINPUT_PLAYER_H

#include <signal.h>
#include "macro.h"

/* Replay with monotonic absolute deadlines. fd is already opened for writing.
 * Emits key/button releases if playback is interrupted or fails.
 * Return value: 0 or negative errno.
 */
int vinput_play_macro(int fd, const struct vinput_macro_event *head,
                      const volatile sig_atomic_t *stop);

#endif
