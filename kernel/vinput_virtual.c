#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sched.h>

#include "vinput_virtual.h"
#include "vinput_input.h"
#include "vinput_ringbuf.h"
#include "uapi/vinput_uapi.h"

static struct input_dev *virtual_devices[VINPUT_VIRTUAL_DEVICES];

static struct task_struct *vinput_virtual_thread;

static struct vinput_ringbuf *vinput_output_rb;
/* Only the virtual worker reads/writes these per-source pending-report flags. */
static bool pending_sync[MAX_INPUT_DEVICES][VINPUT_VIRTUAL_DEVICES];

static int vinput_virtual_create_keyboard(void)
{
    struct input_dev *dev;
    int ret;

    dev = input_allocate_device();
    if (!dev)
        return -ENOMEM;

    dev->name = "vinput virtual keyboard";
    dev->phys = "vinput/keyboard";
    dev->id.bustype = BUS_VIRTUAL;

    __set_bit(EV_KEY, dev->evbit);

    for (unsigned int code = KEY_RESERVED;
         code < KEY_MAX;
         code++) {
        __set_bit(code, dev->keybit);
    }

    ret = input_register_device(dev);
    if (ret) {
        input_free_device(dev);
        return ret;
    }

    virtual_devices[VINPUT_VIRTUAL_KEYBOARD] = dev;

    printk("vinput_virtual: registered virtual keyboard\n");

    return 0;
}

static int vinput_virtual_create_mouse(void)
{
    struct input_dev *dev;
    int ret;

    dev = input_allocate_device();
    if (!dev)
        return -ENOMEM;

    dev->name = "vinput virtual mouse";
    dev->phys = "vinput/mouse";
    dev->id.bustype = BUS_VIRTUAL;

    __set_bit(EV_REL, dev->evbit);

    __set_bit(REL_X, dev->relbit);
    __set_bit(REL_Y, dev->relbit);
    __set_bit(REL_WHEEL, dev->relbit);
    __set_bit(REL_HWHEEL, dev->relbit);

    __set_bit(EV_KEY, dev->evbit);

    __set_bit(BTN_LEFT, dev->keybit);
    __set_bit(BTN_RIGHT, dev->keybit);
    __set_bit(BTN_MIDDLE, dev->keybit);
    __set_bit(BTN_SIDE, dev->keybit);
    __set_bit(BTN_EXTRA, dev->keybit);
    __set_bit(BTN_FORWARD, dev->keybit);
    __set_bit(BTN_BACK, dev->keybit);
    __set_bit(BTN_TASK, dev->keybit);

    ret = input_register_device(dev);
    if (ret) {
        input_free_device(dev);
        return ret;
    }

    virtual_devices[VINPUT_VIRTUAL_MOUSE] = dev;

    printk("vinput_virtual: registered virtual mouse\n");

    return 0;
}

static int vinput_virtual_emit(const struct vinput_event *event)
{
    struct input_dev *dev;
    unsigned int target;

    if (!event || event->dev_id >= MAX_INPUT_DEVICES)
        return -EINVAL;

    if (event->type == EV_SYN) {
        if (event->code != SYN_REPORT)
            return 0;

        for (target = 0; target < VINPUT_VIRTUAL_DEVICES; target++) {
            if (pending_sync[event->dev_id][target]) {
                input_sync(virtual_devices[target]);
                pending_sync[event->dev_id][target] = false;
            }
        }
        return 0;
    }

    if (event->type == EV_REL ||
        (event->type == EV_KEY && event->code >= BTN_MOUSE &&
         event->code < BTN_JOYSTICK)) {
        target = VINPUT_VIRTUAL_MOUSE;
    } else if (event->type == EV_KEY) {
        target = VINPUT_VIRTUAL_KEYBOARD;
    } else {
        return -EOPNOTSUPP;
    }

    dev = virtual_devices[target];
    if (!dev)
        return -ENODEV;

    input_event(dev, event->type, event->code, event->value);
    pending_sync[event->dev_id][target] = true;
    return 0;
}

static int vinput_virtual_worker(void *data)
{
    struct vinput_ringbuf *rb = data;
    struct vinput_event event;
    int ret;

    while (!kthread_should_stop()) {

        ret = vinput_ringbuf_pop(rb, &event);

        if (ret == 0) {
            ret = vinput_virtual_emit(&event);

            if (ret < 0) {
                pr_debug("vinput_virtual: failed to emit event: %d\n",
                         ret);
            }

            continue;
        }

        if (ret == -EAGAIN) {
            wait_event_interruptible(rb->not_empty,
                                     kthread_should_stop() ||
                                     !vinput_ringbuf_is_empty(rb));
            continue;
        }

        pr_warn_ratelimited("vinput_virtual: ring buffer pop failed: %d\n",
                            ret);
    }

    return 0;
}


int vinput_virtual_init(struct vinput_ringbuf *output_rb)
{
    int ret;

    if (!output_rb)
        return -EINVAL;

    memset(pending_sync, 0, sizeof(pending_sync));
    vinput_output_rb = output_rb;

    ret = vinput_virtual_create_keyboard();
    if (ret)
        goto err_reset_rb;

    ret = vinput_virtual_create_mouse();
    if (ret)
        goto err_keyboard;

    vinput_virtual_thread =
        kthread_run(vinput_virtual_worker,
                    vinput_output_rb,
                    "vinput_virtual");

    if (IS_ERR(vinput_virtual_thread)) {
        ret = PTR_ERR(vinput_virtual_thread);
        vinput_virtual_thread = NULL;
        goto err_mouse;
    }

    printk("vinput_virtual: initialized\n");

    return 0;

err_mouse:
    input_unregister_device(
        virtual_devices[VINPUT_VIRTUAL_MOUSE]);

    virtual_devices[VINPUT_VIRTUAL_MOUSE] = NULL;

err_keyboard:
    input_unregister_device(
        virtual_devices[VINPUT_VIRTUAL_KEYBOARD]);

    virtual_devices[VINPUT_VIRTUAL_KEYBOARD] = NULL;

err_reset_rb:
    vinput_output_rb = NULL;

    return ret;
}

void vinput_virtual_exit(void)
{
    unsigned int i;

    if (vinput_virtual_thread) {
        wake_up_interruptible(&vinput_output_rb->not_empty);
        kthread_stop(vinput_virtual_thread);
        vinput_virtual_thread = NULL;
    }

    for (i = 0; i < VINPUT_VIRTUAL_DEVICES; i++) {
        if (virtual_devices[i]) {
            input_unregister_device(virtual_devices[i]);
            virtual_devices[i] = NULL;
        }
    }

    vinput_output_rb = NULL;

    printk("vinput_virtual: exited\n");
}