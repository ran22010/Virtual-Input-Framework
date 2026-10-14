#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/ratelimit.h>

#include "vinput_input.h"
#include "vinput_ringbuf.h"
#include "uapi/vinput_uapi.h"

static struct vinput_device_manager device_manager;
static DEFINE_SPINLOCK(device_manager_lock);
static atomic64_t dropped_events = ATOMIC64_INIT(0);

static struct vinput_ringbuf *vinput_input_rb;

static int vinput_input_get_device_id(struct input_dev *dev)
{
    unsigned long flags;
    unsigned int i;
    int id = -1;

    spin_lock_irqsave(&device_manager_lock, flags);
    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
        if (device_manager.devices[i] == dev) {
            id = i;
            break;
        }
    }
    spin_unlock_irqrestore(&device_manager_lock, flags);
    return id;
}

static int vinput_input_add_device(struct input_dev *dev)
{
    unsigned long flags;
    unsigned int i;
    int id = -ENOSPC;

    spin_lock_irqsave(&device_manager_lock, flags);
    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
        if (!device_manager.devices[i]) {
            device_manager.devices[i] = dev;
            device_manager.count++;
            id = i;
            break;
        }
    }
    spin_unlock_irqrestore(&device_manager_lock, flags);
    return id;
}

static void vinput_input_remove_device(struct input_dev *dev)
{
    unsigned long flags;
    unsigned int i;

    spin_lock_irqsave(&device_manager_lock, flags);
    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
        if (device_manager.devices[i] == dev) {
            device_manager.devices[i] = NULL;
            device_manager.count--;
            break;
        }
    }
    spin_unlock_irqrestore(&device_manager_lock, flags);
}

static bool vinput_input_is_virtual(struct input_dev *dev)
{
    if (dev->id.bustype != BUS_VIRTUAL)
        return false;

    if (dev->phys == NULL)
        return false;

    if (!strcmp(dev->phys, "vinput/keyboard"))
        return true;

    if (!strcmp(dev->phys, "vinput/mouse"))
        return true;

    return false;
}

static int vinput_input_connect(struct input_handler *handler,
                                struct input_dev *dev,
                                const struct input_device_id *id)
{
    struct input_handle *handle;
    int ret;

    if (vinput_input_is_virtual(dev))
        return -ENODEV;

    handle = kzalloc(sizeof(*handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;

    handle->dev = dev;
    handle->handler = handler;
    handle->name = "vinput_input";

    ret = input_register_handle(handle);
    if (ret)
        goto err_free;

    ret = vinput_input_add_device(dev);
    if (ret < 0)
        goto err_unregister;

    ret = input_open_device(handle);
    if (ret)
        goto err_remove_device;

    printk("vinput_input: connected to %s\n",
           dev_name(&dev->dev));

    return 0;

err_remove_device:
    vinput_input_remove_device(dev);

err_unregister:
    input_unregister_handle(handle);

err_free:
    kfree(handle);

    return ret;
}

static void vinput_input_disconnect(struct input_handle *handle)
{
    input_close_device(handle);
    vinput_input_remove_device(handle->dev);
    input_unregister_handle(handle);

    kfree(handle);
}

static void vinput_input_event(struct input_handle *handle,
                               unsigned int type,
                               unsigned int code,
                               int value)
{
    struct vinput_event event;
    int dev_id;

    dev_id = vinput_input_get_device_id(handle->dev);
    if (dev_id < 0)
        return;

    event.timestamp_ns = ktime_get_ns();
    event.dev_id = dev_id;
    event.type = type;
    event.code = code;
    event.reserved = 0;
    event.value = value;

    if (vinput_ringbuf_push(vinput_input_rb, &event) == -ENOSPC) {
        atomic64_inc(&dropped_events);
        pr_warn_ratelimited("vinput_input: dropped %lld events (full ring buffer)\n",
                            (long long)atomic64_read(&dropped_events));
    }
}

static const struct input_device_id vinput_input_ids[] = {
    {
        .driver_info = 1,
    },
    { },
};

static struct input_handler vinput_input_handler = {
    .event      = vinput_input_event,
    .connect    = vinput_input_connect,
    .disconnect = vinput_input_disconnect,
    .name       = "vinput_input",
    .id_table   = vinput_input_ids,
};

int vinput_input_init(struct vinput_ringbuf *input_rb)
{
    int ret;

    if (!input_rb)
        return -EINVAL;

    atomic64_set(&dropped_events, 0);
    vinput_input_rb = input_rb;
    ret = input_register_handler(&vinput_input_handler);
    if (ret)
        vinput_input_rb = NULL;
    return ret;
}

void vinput_input_exit(void)
{
    input_unregister_handler(&vinput_input_handler);
    vinput_input_rb = NULL;
}
