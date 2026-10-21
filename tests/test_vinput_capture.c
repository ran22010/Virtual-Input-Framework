/* End-to-end recording: synthetic uinput device -> input handler -> read(). */
#include "test_common.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int create_test_keyboard(void)
{
    struct uinput_setup setup = {0};
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        if (errno == ENOENT || errno == ENODEV) {
            fprintf(stderr, "[SKIP] /dev/uinput unavailable (%s). Run 'sudo modprobe uinput'\n",
                    strerror(errno));
            return -1;
        }
        fprintf(stderr, "[FAIL] cannot open /dev/uinput: %s (run tests with sudo)\n",
                strerror(errno));
        return -2;
    }
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(fd, UI_SET_KEYBIT, KEY_F24) < 0) {
        perror("uinput key capabilities");
        close(fd);
        return -2;
    }
    snprintf(setup.name, sizeof(setup.name), "vinput test F24 source");
    setup.id.bustype = BUS_USB;
    if (ioctl(fd, UI_DEV_SETUP, &setup) < 0 ||
        ioctl(fd, UI_DEV_CREATE) < 0) {
        perror("uinput create");
        close(fd);
        return -2;
    }
    return fd;
}

static int emit(int fd, unsigned int type, unsigned int code, int value)
{
    struct input_event ev = {0};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    return write(fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev) ? 0 : -1;
}

static int capture_events(int vif_fd)
{
    struct vinput_event records[128];
    int state = 0, dev_id = -1;
    long long deadline = now_ms() + TEST_TIMEOUT_MS;
    while (now_ms() < deadline && state < 4) {
        ssize_t n = read(vif_fd, records, sizeof(records));
        if (n == -1 && errno == EINTR)
            continue;
        if (n == -1 && errno == EAGAIN) {
            usleep(10000);
            continue;
        }
        if (n <= 0 || (size_t)n % sizeof(records[0])) {
            fprintf(stderr, "[FAIL] capture read: %zd (%s)\n", n, strerror(errno));
            return -1;
        }
        for (size_t i = 0; i < (size_t)n / sizeof(records[0]); i++) {
            struct vinput_event *e = &records[i];
            if (e->reserved || !e->timestamp_ns) {
                fprintf(stderr, "[FAIL] malformed captured event\n");
                return -1;
            }
            if (state == 0 && e->type == EV_KEY &&
                e->code == KEY_F24 && e->value == 1) {
                dev_id = (int)e->dev_id;
                state = 1;
            } else if (dev_id == (int)e->dev_id && state == 1 &&
                       e->type == EV_SYN && e->code == SYN_REPORT) {
                state = 2;
            } else if (dev_id == (int)e->dev_id && state == 2 &&
                       e->type == EV_KEY && e->code == KEY_F24 && e->value == 0) {
                state = 3;
            } else if (dev_id == (int)e->dev_id && state == 3 &&
                       e->type == EV_SYN && e->code == SYN_REPORT) {
                state = 4;
            }
        }
    }
    if (state != 4) {
        fprintf(stderr, "[FAIL] captured F24/SYN sequence incomplete (state %d)\n", state);
        return -1;
    }
    printf("[PASS] capture F24 down/SYN/up/SYN, source dev_id=%d\n", dev_id);
    return 0;
}

int main(void)
{
    int vif_fd = open_vif();
    int ui_fd, errors = 0;
    if (vif_fd < 0)
        return 1;
    ui_fd = create_test_keyboard();
    if (ui_fd < 0) {
        close(vif_fd);
        return ui_fd == -1 ? 77 : 1;
    }

    /* Allow the kernel input handler to connect before sending events. */
    usleep(400000);
    if (drain_fd(vif_fd, sizeof(struct vinput_event), 100) != 0) {
        fprintf(stderr, "[FAIL] capture buffer did not drain (busy input source?)\n");
        errors++;
        goto done;
    }

    if (emit(ui_fd, EV_KEY, KEY_F24, 1) ||
        emit(ui_fd, EV_SYN, SYN_REPORT, 0) ||
        emit(ui_fd, EV_KEY, KEY_F24, 0) ||
        emit(ui_fd, EV_SYN, SYN_REPORT, 0)) {
        perror("[FAIL] emitting synthetic events");
        errors++;
        goto done;
    }
    errors += capture_events(vif_fd) != 0;

done:
    if (ioctl(ui_fd, UI_DEV_DESTROY) < 0) {
        perror("uinput destroy");
        errors++;
    }
    close(ui_fd);
    close(vif_fd);
    printf("Input capture: %s (%d failures)\n", errors ? "FAIL" : "PASS", errors);
    return errors ? 1 : 0;
}
