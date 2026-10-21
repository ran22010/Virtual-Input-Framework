#define _POSIX_C_SOURCE 200809L
#include "macro_file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FORMAT_VERSION 1U
static const char FILE_MAGIC[8] = {'V', 'I', 'F', 'M', 'A', 'C', '0', '1'};

struct macro_file_header {
    char magic[8];
    uint32_t version;
    uint32_t event_size;
    uint64_t event_count;
};

_Static_assert(sizeof(struct macro_file_header) == 24, "file header must be 24 bytes");
_Static_assert(sizeof(struct vinput_event) == 24, "unexpected vinput_event ABI size");

int macro_file_save(const char *path, const struct vinput_event *events,
                    size_t count)
{
    struct macro_file_header header = {0};
    FILE *f;
    int fd, err = 0;
    size_t i;

    if (!path || !events || count == 0 || count > VINPUT_MAX_EVENTS)
        return -EINVAL;
    for (i = 0; i < count; ++i) {
        if (!vinput_event_playable(&events[i]) ||
            (i && events[i].timestamp_ns < events[i - 1].timestamp_ns))
            return -EINVAL;
    }

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0)
        return -errno;
    if (fchmod(fd, 0600) < 0) {
        err = -errno;
        close(fd);
        return err;
    }
    f = fdopen(fd, "wb");
    if (!f) {
        err = -errno;
        close(fd);
        return err;
    }

    memcpy(header.magic, FILE_MAGIC, sizeof(FILE_MAGIC));
    header.version = FORMAT_VERSION;
    header.event_size = (uint32_t)sizeof(*events);
    header.event_count = (uint64_t)count;

    if (fwrite(&header, sizeof(header), 1, f) != 1 ||
        fwrite(events, sizeof(*events), count, f) != count) {
        err = errno ? -errno : -EIO;
    }
    if (fclose(f) != 0 && err == 0)
        err = -errno;
    return err;
}

int macro_file_load(const char *path, struct vinput_event **events_out,
                    size_t *count_out)
{
    struct macro_file_header header;
    struct vinput_event *events = NULL;
    FILE *f;
    int err = 0;
    size_t count, i;

    if (!path || !events_out || !count_out)
        return -EINVAL;
    *events_out = NULL;
    *count_out = 0;
    f = fopen(path, "rb");
    if (!f)
        return -errno;

    if (fread(&header, sizeof(header), 1, f) != 1) {
        err = -EINVAL;
        goto done;
    }
    if (memcmp(header.magic, FILE_MAGIC, sizeof(FILE_MAGIC)) ||
        header.version != FORMAT_VERSION ||
        header.event_size != sizeof(struct vinput_event) ||
        header.event_count == 0 || header.event_count > VINPUT_MAX_EVENTS) {
        err = -EINVAL;
        goto done;
    }

    count = (size_t)header.event_count;
    events = calloc(count, sizeof(*events));
    if (!events) {
        err = -ENOMEM;
        goto done;
    }
    if (fread(events, sizeof(*events), count, f) != count || fgetc(f) != EOF) {
        err = -EINVAL;
        goto done;
    }
    if (ferror(f)) {
        err = -EIO;
        goto done;
    }
    for (i = 0; i < count; ++i) {
        if (!vinput_event_playable(&events[i]) ||
            (i && events[i].timestamp_ns < events[i - 1].timestamp_ns)) {
            err = -EINVAL;
            goto done;
        }
    }
    *events_out = events;
    *count_out = count;
    events = NULL;

done:
    free(events);
    if (fclose(f) != 0 && !err)
        err = -errno;
    return err;
}
