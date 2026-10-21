/* End-to-end replay: /dev/vif_driver -> ring buffer -> virtual evdev. */
#include "test_common.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int open_evdev(const char *name)
{
    char path[128];
    int fd;
    if (find_evdev_by_name(name, path, sizeof(path)) != 0) {
        fprintf(stderr, "[FAIL] Cannot find '%s' in /sys/class/input\n", name);
        return -1;
    }
    fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
        fprintf(stderr, "[FAIL] Cannot open %s: %s\n", path, strerror(errno));
    return fd;
}

/* Expect an ordered sequence of (event, SYN_REPORT, event, SYN_REPORT).
   Unrelated evdev events can occur, but must not satisfy the sequence. */
static int observe_sequence(int fd, unsigned int type, unsigned int code,
                            int first_value, int second_value)
{
    int state = 0;
    long long deadline = now_ms() + TEST_TIMEOUT_MS;
    while (now_ms() < deadline && state < 4) {
        struct pollfd pfd = {.fd = fd, .events = POLLIN};
        struct input_event received[64];
        int remaining = (int)(deadline - now_ms());
        int pr = poll(&pfd, 1, remaining > 0 ? remaining : 0);
        if (pr < 0 && errno == EINTR)
            continue;
        if (pr <= 0)
            break;
        ssize_t n = read(fd, received, sizeof(received));
        if (n < 0 && (errno == EINTR || errno == EAGAIN))
            continue;
        if (n <= 0 || (size_t)n % sizeof(received[0]))
            break;
        for (size_t i = 0; i < (size_t)n / sizeof(received[0]); i++) {
            struct input_event *ev = &received[i];
            if (state == 0 && ev->type == type && ev->code == code &&
                ev->value == first_value)
                state = 1;
            else if (state == 1 && ev->type == EV_SYN &&
                     ev->code == SYN_REPORT)
                state = 2;
            else if (state == 2 && ev->type == type &&
                     ev->code == code && ev->value == second_value)
                state = 3;
            else if (state == 3 && ev->type == EV_SYN &&
                     ev->code == SYN_REPORT)
                state = 4;
        }
    }
    return state == 4 ? 0 : -1;
}

static int check_device(int vif_fd, int evdev_fd, const char *description,
                        unsigned int type, unsigned int code, int first, int second)
{
    struct vinput_event events[] = {
        make_vinput_event(0, type, code, first),
        make_vinput_event(0, EV_SYN, SYN_REPORT, 0),
        make_vinput_event(0, type, code, second),
        make_vinput_event(0, EV_SYN, SYN_REPORT, 0),
    };
    if (check_count(description, write(vif_fd, events, sizeof(events)),
                    sizeof(events)))
        return 1;
    if (observe_sequence(evdev_fd, type, code, first, second) != 0) {
        fprintf(stderr, "[FAIL] %s: expected event/SYN sequence not observed\n",
                description);
        return 1;
    }
    printf("[PASS] %s: evdev received both events and SYN_REPORT\n", description);
    return 0;
}

int main(void)
{
    int vif_fd = open_vif(), kbd_fd, mouse_fd, errors = 0;
    if (vif_fd < 0)
        return 1;
    kbd_fd = open_evdev("vinput virtual keyboard");
    mouse_fd = open_evdev("vinput virtual mouse");
    if (kbd_fd < 0 || mouse_fd < 0) {
        if (kbd_fd >= 0) close(kbd_fd);
        if (mouse_fd >= 0) close(mouse_fd);
        close(vif_fd);
        return 1;
    }

    /* F24 has no ordinary text effect; release is always sent.
       Mouse moves one pixel and back (zero net displacement). */
    errors += check_device(vif_fd, kbd_fd, "keyboard F24 down/up",
                           EV_KEY, KEY_F24, 1, 0);
    errors += check_device(vif_fd, mouse_fd, "mouse REL_X +1/-1",
                           EV_REL, REL_X, 1, -1);

    close(kbd_fd);
    close(mouse_fd);
    close(vif_fd);
    printf("Output pipeline: %s (%d failures)\n", errors ? "FAIL" : "PASS", errors);
    return errors ? 1 : 0;
}
