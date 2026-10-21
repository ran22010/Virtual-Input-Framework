#define _POSIX_C_SOURCE 200809L
#include "recorder.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DEVICE_SLOTS 256U
#define READ_BATCH 128U

struct input_source {
    unsigned char keyboard_pending;
    unsigned char mouse_pending;
};

struct recorder_state {
    struct vinput_event *events;
    size_t length;
    size_t capacity;
    __u64 first_timestamp;
    __u64 last_timestamp;
    struct input_source sources[DEVICE_SLOTS];
};

static int monotonic_time_ns(__u64 *out)
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) < 0)
        return -errno;
    *out = (__u64)t.tv_sec * 1000000000ULL + (__u64)t.tv_nsec;
    return 0;
}

static int append_event(struct recorder_state *state, struct vinput_event event)
{
    if (state->length == state->capacity)
        return -ENOSPC;

    if (state->length == 0)
        state->first_timestamp = event.timestamp_ns;
    if (event.timestamp_ns < state->first_timestamp)
        event.timestamp_ns = state->first_timestamp;
    event.timestamp_ns -= state->first_timestamp;
    if (state->length && event.timestamp_ns < state->last_timestamp)
        event.timestamp_ns = state->last_timestamp;
    state->last_timestamp = event.timestamp_ns;
    event.reserved = 0;
    state->events[state->length++] = event;
    return 0;
}

static int process_event(struct recorder_state *state,
                         const struct vinput_event *raw)
{
    struct vinput_event e = {0};
    struct input_source *src;
    int ret;

    if (raw->dev_id >= DEVICE_SLOTS)
        return 0;
    src = &state->sources[raw->dev_id];
    e.timestamp_ns = raw->timestamp_ns;
    e.dev_id = raw->dev_id;
    e.type = raw->type;
    e.code = raw->code;
    e.value = raw->value;

    if (e.type == EV_KEY) {
        if (e.value < 0 || e.value > 2)
            return 0;
        if (e.code == BTN_LEFT || e.code == BTN_RIGHT || e.code == BTN_MIDDLE) {
            if (e.value == 2)
                return 0;
            e.dev_id = VINPUT_MOUSE_ID;
            src->mouse_pending = 1;
        } else if (e.code > KEY_RESERVED && e.code < KEY_MAX &&
                   e.code < BTN_MISC) {
            e.dev_id = VINPUT_KEYBOARD_ID;
            src->keyboard_pending = 1;
        } else {
            return 0;
        }
        return append_event(state, e);
    }

    if (e.type == EV_REL && (e.code == REL_X || e.code == REL_Y)) {
        e.dev_id = VINPUT_MOUSE_ID;
        src->mouse_pending = 1;
        return append_event(state, e);
    }

    if (e.type == EV_SYN && e.code == SYN_REPORT) {
        e.value = 0;
        if (src->keyboard_pending) {
            e.dev_id = VINPUT_KEYBOARD_ID;
            ret = append_event(state, e);
            if (ret)
                return ret;
            src->keyboard_pending = 0;
        }
        if (src->mouse_pending) {
            e.dev_id = VINPUT_MOUSE_ID;
            ret = append_event(state, e);
            if (ret)
                return ret;
            src->mouse_pending = 0;
        }
    }
    /* EV_MSC, EV_ABS, device metadata, and other unsupported events are skipped. */
    return 0;
}

/* Complete any partial reports if recording ends between an event and SYN. */
static int finalize_reports(struct recorder_state *state)
{
    for (unsigned int source = 0; source < DEVICE_SLOTS; ++source) {
        struct input_source *src = &state->sources[source];
        struct vinput_event e = {0};
        int ret;

        if (!src->keyboard_pending && !src->mouse_pending)
            continue;
        e.timestamp_ns = state->first_timestamp + state->last_timestamp;
        e.type = EV_SYN;
        e.code = SYN_REPORT;
        if (src->keyboard_pending) {
            e.dev_id = VINPUT_KEYBOARD_ID;
            ret = append_event(state, e);
            if (ret)
                return ret;
        }
        if (src->mouse_pending) {
            e.dev_id = VINPUT_MOUSE_ID;
            ret = append_event(state, e);
            if (ret)
                return ret;
        }
    }
    return 0;
}

int vinput_record(const char *device_path, unsigned int duration_seconds,
                  size_t max_events, const volatile sig_atomic_t *stop,
                  struct vinput_event **events_out, size_t *count_out)
{
    struct recorder_state *state;
    __u64 start_ns = 0, now_ns = 0, deadline_ns;
    int fd = -1, ret = 0;

    if (!device_path || !events_out || !count_out || !duration_seconds ||
        max_events == 0 || max_events > VINPUT_MAX_EVENTS)
        return -EINVAL;
    *events_out = NULL;
    *count_out = 0;

    state = calloc(1, sizeof(*state));
    if (!state)
        return -ENOMEM;
    state->events = calloc(max_events, sizeof(*state->events));
    if (!state->events) {
        free(state);
        return -ENOMEM;
    }
    state->capacity = max_events;

    fd = open(device_path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        ret = -errno;
        goto done;
    }

    /* Discard stale captures already buffered before recording begins. */
    for (int attempts = 0; attempts < 128; ++attempts) {
        struct vinput_event ignored[READ_BATCH];
        ssize_t n = read(fd, ignored, sizeof(ignored));
        if (n <= 0) {
            if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                ret = -errno;
                goto done;
            }
            break;
        }
    }

    ret = monotonic_time_ns(&start_ns);
    if (ret)
        goto done;
    deadline_ns = start_ns + (__u64)duration_seconds * 1000000000ULL;

    while (!stop || !*stop) {
        struct vinput_event batch[READ_BATCH];
        ssize_t n;
        size_t i, count;

        ret = monotonic_time_ns(&now_ns);
        if (ret || now_ns >= deadline_ns)
            break;
        n = read(fd, batch, sizeof(batch));
        if (n > 0) {
            if ((size_t)n % sizeof(batch[0]) != 0) {
                ret = -EPROTO;
                break;
            }
            count = (size_t)n / sizeof(batch[0]);
            for (i = 0; i < count; ++i) {
                ret = process_event(state, &batch[i]);
                if (ret)
                    break;
            }
            if (ret)
                break;
            continue;
        }
        if (n == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
            __u64 remaining = deadline_ns - now_ns;
            __u64 ns = remaining < 5000000ULL ? remaining : 5000000ULL;
            struct timespec pause = { .tv_sec = 0, .tv_nsec = (long)ns };
            nanosleep(&pause, NULL); /* no .poll support in the current driver */
            continue;
        }
        if (errno == EINTR)
            continue;
        ret = -errno;
        break;
    }

    if (!ret && state->length != 0)
        ret = finalize_reports(state);
    if (!ret && state->length == 0)
        ret = -ENODATA;
    if (!ret) {
        *events_out = state->events;
        *count_out = state->length;
        state->events = NULL;
    }

done:
    if (fd >= 0)
        close(fd);
    free(state->events);
    free(state);
    return ret;
}
