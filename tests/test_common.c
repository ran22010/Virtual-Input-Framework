#include "test_common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

long long now_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return -1;
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int open_vif(void)
{
    int fd = open(VIF_PATH, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0)
        fprintf(stderr, "Cannot open %s: %s (load vinput.ko and use sudo)\n",
                VIF_PATH, strerror(errno));
    return fd;
}

struct vinput_event make_vinput_event(unsigned int source_id,
                                      unsigned int type,
                                      unsigned int code, int value)
{
    struct vinput_event e = {0};
    e.dev_id = source_id;
    e.type = type;
    e.code = code;
    e.value = value;
    return e;
}

int check_errno(const char *description, ssize_t result, int expected_errno)
{
    int got_errno = errno;
    if (result == -1 && got_errno == expected_errno) {
        printf("[PASS] %s: %s\n", description, strerror(expected_errno));
        return 0;
    }
    fprintf(stderr, "[FAIL] %s: return=%zd, errno=%s; expected -1/%s\n",
            description, result, strerror(got_errno), strerror(expected_errno));
    return 1;
}

int check_count(const char *description, ssize_t result, size_t expected)
{
    if (result >= 0 && (size_t)result == expected) {
        printf("[PASS] %s: %zu bytes\n", description, expected);
        return 0;
    }
    fprintf(stderr, "[FAIL] %s: returned %zd bytes (expected %zu), errno=%s\n",
            description, result, expected, strerror(errno));
    return 1;
}

int find_evdev_by_name(const char *name, char *out, size_t out_size)
{
    for (int n = 0; n < 512; ++n) {
        char sysfs[128], found_name[256];
        FILE *f;
        int len;

        snprintf(sysfs, sizeof(sysfs),
                 "/sys/class/input/event%d/device/name", n);
        f = fopen(sysfs, "r");
        if (!f)
            continue;
        if (!fgets(found_name, sizeof(found_name), f)) {
            fclose(f);
            continue;
        }
        fclose(f);
        found_name[strcspn(found_name, "\r\n")] = '\0';
        if (strcmp(found_name, name) != 0)
            continue;
        len = snprintf(out, out_size, "/dev/input/event%d", n);
        return len >= 0 && (size_t)len < out_size ? 0 : -1;
    }
    return -1;
}

/* Returns 0 when drained to EAGAIN, -1 when input never became idle or failed. */
int drain_fd(int fd, size_t record_size, int max_batches)
{
    unsigned char bytes[4096];
    if (record_size == 0)
        return -1;
    for (int i = 0; i < max_batches; i++) {
        ssize_t n = read(fd, bytes, (sizeof(bytes) / record_size) * record_size);
        if (n == -1 && errno == EINTR) {
            i--;
            continue;
        }
        if (n == -1 && errno == EAGAIN)
            return 0;
        if (n <= 0 || (size_t)n % record_size)
            return -1;
    }
    return -1;
}
