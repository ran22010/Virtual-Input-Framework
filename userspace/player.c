#define _POSIX_C_SOURCE 200809L
#include "player.h"

#include <errno.h>
#include <linux/input-event-codes.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#define NS_PER_SEC 1000000000ULL
#define MAX_PLAYBACK_NS (24ULL * 60ULL * 60ULL * NS_PER_SEC)

static int monotonic_now(struct timespec *ts)
{
    return clock_gettime(CLOCK_MONOTONIC, ts) == 0 ? 0 : -errno;
}

static int submit_event(int fd, const struct vinput_event *event,
                        const volatile sig_atomic_t *stop)
{
    unsigned int retries = 0;
    for (;;) {
        ssize_t n;
        if (stop && *stop)
            return -EINTR;
        n = write(fd, event, sizeof(*event));
        if (n == (ssize_t)sizeof(*event))
            return 0;
        if (n >= 0)
            return -EIO; /* Driver should accept exactly one entire record. */
        if (errno == EINTR)
            continue;
        if ((errno == ENOSPC || errno == EAGAIN) && retries++ < 5000U) {
            struct timespec pause = { .tv_sec = 0, .tv_nsec = 1000000L };
            nanosleep(&pause, NULL);
            continue;
        }
        return -(errno == ENOSPC || errno == EAGAIN ? ETIMEDOUT : errno);
    }
}

/* Regardless of the stop flag, try to release keys on cancellation.
 * Bounded retries are handled by submit_event(). */
static void release_pressed(int fd, const bool held[2][KEY_MAX + 1])
{
    for (unsigned int device = 0; device < 2; ++device) {
        bool any = false;
        for (unsigned int code = 0; code <= KEY_MAX; ++code) {
            if (!held[device][code])
                continue;
            struct vinput_event up = {0};
            up.dev_id = device;
            up.type = EV_KEY;
            up.code = code;
            up.value = 0;
            if (!submit_event(fd, &up, NULL))
                any = true;
        }
        if (any) {
            struct vinput_event sync = {0};
            sync.dev_id = device;
            sync.type = EV_SYN;
            sync.code = SYN_REPORT;
            (void)submit_event(fd, &sync, NULL);
        }
    }
}

int vinput_play_macro(int fd, const struct vinput_macro_event *head,
                      const volatile sig_atomic_t *stop)
{
    struct timespec start, target;
    const struct vinput_macro_event *curr;
    __u64 cumulative = 0;
    bool held[2][KEY_MAX + 1] = {{false}};
    int ret;

    if (fd < 0 || !head)
        return -EINVAL;
    ret = monotonic_now(&start);
    if (ret)
        return ret;

    for (curr = head; curr; curr = curr->next) {
        struct vinput_event event = curr->event;
        int sleep_ret;

        if (!vinput_event_playable(&event) ||
            curr->delay_ns > MAX_PLAYBACK_NS - cumulative) {
            ret = -EINVAL;
            break;
        }
        cumulative += curr->delay_ns;
        target.tv_sec = start.tv_sec + (time_t)(cumulative / NS_PER_SEC);
        target.tv_nsec = start.tv_nsec + (long)(cumulative % NS_PER_SEC);
        if (target.tv_nsec >= (long)NS_PER_SEC) {
            target.tv_sec++;
            target.tv_nsec -= (long)NS_PER_SEC;
        }

        for (;;) {
            if (stop && *stop) {
                ret = -EINTR;
                break;
            }
            sleep_ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
                                        &target, NULL);
            if (sleep_ret == 0)
                break;
            if (sleep_ret != EINTR) {
                ret = -sleep_ret;
                break;
            }
        }
        if (ret)
            break;

        {
            struct timespec now;
            ret = monotonic_now(&now);
            if (ret)
                break;
            event.timestamp_ns = (__u64)now.tv_sec * NS_PER_SEC + (__u64)now.tv_nsec;
        }
        ret = submit_event(fd, &event, stop);
        if (ret)
            break;

        if (event.type == EV_KEY && event.code <= KEY_MAX) {
            if (event.value == 1)
                held[event.dev_id][event.code] = true;
            else if (event.value == 0)
                held[event.dev_id][event.code] = false;
        }
    }

    /* Ensure a macro ending on a key-down does not leave it stuck. */
    release_pressed(fd, held);
    return ret;
}
