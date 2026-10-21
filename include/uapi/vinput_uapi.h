#ifndef UAPI_VINPUT_UAPI_H
#define UAPI_VINPUT_UAPI_H

#include <linux/types.h>

struct vinput_event {
    __u64 timestamp_ns;
    __u16 dev_id;
    __u16 type;
    __u16 code;
    __u16 reserved;
    __s32 value;
};

#endif /* _UAPI_VINPUT_UAPI_H */