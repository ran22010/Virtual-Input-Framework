#define _POSIX_C_SOURCE 200809L
#include "macro.h"
#include "macro_file.h"
#include "player.h"
#include "recorder.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_DEVICE "/dev/vif_driver"

static volatile sig_atomic_t interrupted;

static void signal_stop(int signum)
{
    (void)signum;
    interrupted = 1;
}

static void usage(const char *program)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s record --output FILE --duration SECONDS [--device PATH] [--max-events N]\n"
            "  %s play FILE [--device PATH]\n"
            "  %s info FILE\n"
            "\n"
            "SECONDS: 1-3600; N: 1-%u; default device: %s\n"
            "Use Ctrl+C to stop recording or cancel playback.\n",
            program, program, program, VINPUT_MAX_EVENTS, DEFAULT_DEVICE);
}

static int parse_uint(const char *s, unsigned long min, unsigned long max,
                      unsigned long *out)
{
    char *end;
    unsigned long num;
    errno = 0;
    if (!s || *s == '\0' || *s == '-')
        return -EINVAL;
    num = strtoul(s, &end, 10);
    if (errno || *end != '\0' || num < min || num > max)
        return -EINVAL;
    *out = num;
    return 0;
}

static int command_record(int argc, char **argv)
{
    const char *file = NULL, *device = DEFAULT_DEVICE;
    unsigned long duration = 0, max_events = VINPUT_MAX_EVENTS;
    struct vinput_event *events = NULL;
    size_t count = 0;
    int ret;

    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "--output") && i + 1 < argc)
            file = argv[++i];
        else if (!strcmp(argv[i], "--device") && i + 1 < argc)
            device = argv[++i];
        else if (!strcmp(argv[i], "--duration") && i + 1 < argc) {
            if (parse_uint(argv[++i], 1, 3600, &duration))
                return -EINVAL;
        } else if (!strcmp(argv[i], "--max-events") && i + 1 < argc) {
            if (parse_uint(argv[++i], 1, VINPUT_MAX_EVENTS, &max_events))
                return -EINVAL;
        } else {
            return -EINVAL;
        }
    }
    if (!file || duration == 0)
        return -EINVAL;
    fprintf(stderr, "Recording up to %lu seconds. Press Ctrl+C to finish.\n", duration);
    ret = vinput_record(device, (unsigned int)duration, (size_t)max_events,
                        &interrupted, &events, &count);
    if (ret)
        return ret;

    ret = macro_file_save(file, events, count);
    if (!ret)
        printf("Saved %zu normalized events to %s\n", count, file);
    free(events);
    return ret;
}

static int command_play(int argc, char **argv)
{
    const char *file, *device = DEFAULT_DEVICE;
    struct vinput_event *events = NULL;
    struct vinput_macros *macros = NULL;
    size_t count;
    int fd = -1, ret;

    if (argc < 3)
        return -EINVAL;
    file = argv[2];
    for (int i = 3; i < argc; ++i) {
        if (!strcmp(argv[i], "--device") && i + 1 < argc)
            device = argv[++i];
        else
            return -EINVAL;
    }

    ret = macro_file_load(file, &events, &count);
    if (ret)
        goto done;
    macros = calloc(1, sizeof(*macros));
    if (!macros) {
        ret = -ENOMEM;
        goto done;
    }
    ret = vinput_create_macro(macros, events, count, 0);
    if (ret)
        goto done;

    fd = open(device, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        ret = -errno;
        goto done;
    }
    fprintf(stderr, "Playing %zu events from %s. Ctrl+C cancels.\n", count, file);
    ret = vinput_play_macro(fd, macros->heads[0], &interrupted);
    if (!ret)
        puts("Playback complete.");

done:
    if (fd >= 0)
        close(fd);
    if (macros) {
        vinput_clear_macros(macros);
        free(macros);
    }
    free(events);
    return ret;
}

static int command_info(int argc, char **argv)
{
    struct vinput_event *events = NULL;
    size_t count = 0;
    int ret;
    if (argc != 3)
        return -EINVAL;
    ret = macro_file_load(argv[2], &events, &count);
    if (!ret) {
        unsigned long long duration =
            (unsigned long long)(events[count - 1].timestamp_ns - events[0].timestamp_ns);
        printf("File: %s\nEvents: %zu\nDuration: %.3f seconds\n",
               argv[2], count, (double)duration / 1.0e9);
    }
    free(events);
    return ret;
}

int main(int argc, char **argv)
{
    struct sigaction sa = {0};
    int ret;

    if (argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
        usage(argv[0]);
        return argc < 2 ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    umask(077); /* Macro files may contain sensitive keystroke recordings. */
    sa.sa_handler = signal_stop;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (!strcmp(argv[1], "record"))
        ret = command_record(argc, argv);
    else if (!strcmp(argv[1], "play"))
        ret = command_play(argc, argv);
    else if (!strcmp(argv[1], "info"))
        ret = command_info(argc, argv);
    else
        ret = -EINVAL;

    if (ret) {
        fprintf(stderr, "vinput: %s\n", strerror(-ret));
        if (ret == -EINVAL)
            usage(argv[0]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
