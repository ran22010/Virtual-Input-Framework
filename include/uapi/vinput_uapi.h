#ifndef UAPI_VINPUT_UAPI_H
#define UAPI_VINPUT_UAPI_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
typedef uint64_t __u64;
typedef uint32_t __u32;
typedef uint16_t __u16;
typedef int32_t  __s32;
#endif

struct vinput_event {
    __u64 timestamp_ns;
    __u16 dev_id;
    __u16 type;
    __u16 code;
    __u16 reserved;
    __s32 value;
};

#endif /* _UAPI_VINPUT_UAPI_H */