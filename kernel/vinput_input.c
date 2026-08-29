#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ktime.h>

#include "vinput_input.h"
#include "uapi/vinput_uapi.h"

static struct vinput_device_manager device_manager;

static int vinput_input_get_device_id(struct input_dev *dev)
{
    unsigned int i;

    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
        if (device_manager.devices[i] == dev)
            return i;
    }

    return -1;
}

static int vinput_input_add_device(struct input_dev *dev)
{
    unsigned int i;

    for (i = 0; i < MAX_INPUT_DEVICES; i++) {
        if (device_manager.devices[i] == NULL) {
            device_manager.devices[i] = dev;
            return i;
        }
    }

    return -ENOSPC;
}

static void vinput_input_remove_device(struct input_dev *dev)
{
    int dev_id;

    dev_id = vinput_input_get_device_id(dev);
    if (dev_id >= 0)
        device_manager.devices[dev_id] = NULL;
}

static int vinput_input_connect(struct input_handler *handler,
                                struct input_dev *dev,
                                const struct input_device_id *id)
{
    struct input_handle *handle;
    int ret;

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

    pr_info("vinput_input: connected to %s\n",
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

    vinput_ringbuf_push(&event);

    pr_debug("vinput_input: %s: type=%u code=%u value=%d\n",
             dev_name(&handle->dev->dev),
             type,
             code,
             value);
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

int vinput_input_init(void)
{
    return input_register_handler(&vinput_input_handler);
}

void vinput_input_exit(void)
{
    input_unregister_handler(&vinput_input_handler);
}
