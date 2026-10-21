/* Character-device read/write API validation. Does not produce key presses. */
#include "test_common.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int rejected_write(int fd, const char *name, struct vinput_event e)
{
    errno = 0;
    return check_errno(name, write(fd, &e, sizeof(e)), EINVAL);
}

int main(void)
{
    int fd = open_vif(), errors = 0;
    char one = 0;
    struct vinput_event e, batch[2];
    ssize_t n;

    if (fd < 0)
        return 1;
    printf("Testing %s, event record size=%zu bytes\n",
           VIF_PATH, sizeof(struct vinput_event));

    errno = 0;
    errors += check_count("read of zero bytes", read(fd, &one, 0), 0);
    errno = 0;
    errors += check_count("write of zero bytes", write(fd, &one, 0), 0);

    errno = 0;
    errors += check_errno("read with incomplete-record buffer",
                          read(fd, &one, sizeof(one)), EINVAL);
    errno = 0;
    errors += check_errno("write with incomplete-record count",
                          write(fd, &one, sizeof(one)), EINVAL);

    errno = 0;
    errors += check_errno("unsupported ioctl",
                          ioctl(fd, 0x76543210UL, 0), ENOTTY);
    errno = 0;
    errors += check_errno("nonseekable character device",
                          lseek(fd, 0, SEEK_SET), ESPIPE);

    e = make_vinput_event(0, EV_KEY, KEY_F24, 1);
    e.reserved = 1;
    errors += rejected_write(fd, "nonzero reserved field", e);

    e = make_vinput_event(256, EV_KEY, KEY_F24, 1);
    errors += rejected_write(fd, "device ID >= 256", e);

    e = make_vinput_event(0, EV_ABS, ABS_X, 1);
    errors += rejected_write(fd, "unsupported event type", e);

    e = make_vinput_event(0, EV_SYN, SYN_DROPPED, 0);
    errors += rejected_write(fd, "unsupported EV_SYN code", e);

    e = make_vinput_event(0, EV_SYN, SYN_REPORT, 1);
    errors += rejected_write(fd, "nonzero SYN_REPORT value", e);

    e = make_vinput_event(0, EV_REL, REL_Z, 1);
    errors += rejected_write(fd, "unsupported REL code", e);

    e = make_vinput_event(0, EV_KEY, KEY_RESERVED, 1);
    errors += rejected_write(fd, "KEY_RESERVED is rejected", e);

    e = make_vinput_event(0, EV_KEY, KEY_F24, 3);
    errors += rejected_write(fd, "invalid key value 3", e);

    e = make_vinput_event(0, EV_KEY, KEY_F24, -1);
    errors += rejected_write(fd, "invalid negative key value", e);

    e = make_vinput_event(0, EV_KEY, BTN_LEFT, 2);
    errors += rejected_write(fd, "mouse button repeat value 2", e);

    /* A SYN-only write is valid, harmless, and does not cause a key press. */
    batch[0] = make_vinput_event(0, EV_SYN, SYN_REPORT, 0);
    errno = 0;
    errors += check_count("valid single event", write(fd, batch, sizeof(batch[0])),
                          sizeof(batch[0]));

    batch[1] = make_vinput_event(0, EV_SYN, SYN_REPORT, 0);
    batch[1].reserved = 1;
    errno = 0;
    errors += check_count("partial batch stops on invalid second record",
                          write(fd, batch, sizeof(batch)), sizeof(batch[0]));

    /* No other process should be feeding the capture buffer during this check. */
    if (drain_fd(fd, sizeof(e), 100) == 0) {
        errno = 0;
        n = read(fd, &e, sizeof(e));
        errors += check_errno("empty nonblocking read", n, EAGAIN);
    } else {
        printf("[SKIP] empty nonblocking read: capture stream remained active\n");
    }

    close(fd);
    printf("IO validation: %s (%d failures)\n", errors ? "FAIL" : "PASS", errors);
    return errors ? 1 : 0;
}
