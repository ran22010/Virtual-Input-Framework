#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>      // ADDED
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/input.h>

#include "vinput_ringbuf.h"
#include "vinput_input.h"
#include "vinput_virtual.h"

struct my_device_data {
    struct cdev cdev;
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raul");
MODULE_DESCRIPTION("Virtual Input Framework");

/* ADDED */
static dev_t dev_num;
static struct class *vif_class;
static DEFINE_MUTEX(vif_read_lock);
static DEFINE_MUTEX(vif_write_lock);
static struct vinput_ringbuf input_rb;
static struct vinput_ringbuf output_rb;
/* Keep a popped event until copy_to_user succeeds. */
static struct vinput_event pending_read_event;
static bool pending_read_valid;

static struct my_device_data my_device;

/* The device is a stream of complete struct vinput_event records. */
static ssize_t custom_read(struct file *file_pointer,
                           char __user *user_space_buffer,
                           size_t count,
                           loff_t *offset)
{
    const size_t record_size = sizeof(struct vinput_event);
    size_t copied = 0;
    int ret;

    if (!count)
        return 0;
    if (count < record_size)
        return -EINVAL;

    /* vinput_ringbuf_pop() supports only one consumer at a time. */
    ret = mutex_lock_interruptible(&vif_read_lock);
    if (ret)
        return ret;

    while (copied + record_size <= count) {
        if (!pending_read_valid) {
            ret = vinput_ringbuf_pop(&input_rb, &pending_read_event);
            if (ret == -EAGAIN) {
                if (copied)
                    break;
                if (file_pointer->f_flags & O_NONBLOCK)
                    goto out;

                ret = wait_event_interruptible(input_rb.not_empty,
                        !vinput_ringbuf_is_empty(&input_rb));
                if (ret)
                    goto out;
                continue;
            }
            if (ret)
                goto out;
            pending_read_valid = true;
        }

        if (copy_to_user(user_space_buffer + copied,
                         &pending_read_event, record_size)) {
            ret = -EFAULT;
            goto out;
        }

        pending_read_valid = false;
        copied += record_size;
    }
    ret = copied;
out:
    mutex_unlock(&vif_read_lock);
    return copied ? (ssize_t)copied : ret;
}

static bool vinput_valid_output_event(const struct vinput_event *event)
{
    if (event->dev_id >= MAX_INPUT_DEVICES || event->reserved)
        return false;

    switch (event->type) {
    case EV_SYN:
        return event->code == SYN_REPORT && event->value == 0;
    case EV_REL:
        return event->code == REL_X || event->code == REL_Y ||
               event->code == REL_WHEEL || event->code == REL_HWHEEL;
    case EV_KEY:
        if (event->code <= KEY_RESERVED || event->code >= KEY_MAX ||
            event->value < 0 || event->value > 2)
            return false;

        /* Match the mouse buttons advertised by vinput_virtual.c. */
        if (event->code >= BTN_MOUSE && event->code < BTN_JOYSTICK) {
            if (event->value > 1)
                return false;
            switch (event->code) {
            case BTN_LEFT:
            case BTN_RIGHT:
            case BTN_MIDDLE:
            case BTN_SIDE:
            case BTN_EXTRA:
            case BTN_FORWARD:
            case BTN_BACK:
            case BTN_TASK:
                return true;
            default:
                return false;
            }
        }
        return true;
    default:
        return false;
    }
}

static ssize_t custom_write(struct file *file_pointer,
                            const char __user *user_space_buffer,
                            size_t count,
                            loff_t *offset)
{
    const size_t record_size = sizeof(struct vinput_event);
    struct vinput_event event;
    size_t written = 0;
    int ret;

    if (!count)
        return 0;
    if (count % record_size)
        return -EINVAL;

    /* Prevent interleaving batches from separate writers. */
    ret = mutex_lock_interruptible(&vif_write_lock);
    if (ret)
        return ret;

    while (written < count) {
        if (copy_from_user(&event, user_space_buffer + written,
                           record_size)) {
            ret = -EFAULT;
            break;
        }
        if (!vinput_valid_output_event(&event)) {
            ret = -EINVAL;
            break;
        }

        ret = vinput_ringbuf_push(&output_rb, &event);
        if (ret)
            break;
        written += record_size;
    }

    mutex_unlock(&vif_write_lock);
    return written ? (ssize_t)written : ret;
}

static int custom_open(struct inode *inode,
                       struct file *file_pointer)
{
    struct my_device_data *my_data;

    my_data = container_of(inode->i_cdev,
                           struct my_device_data,
                           cdev);

    file_pointer->private_data = my_data;

    return nonseekable_open(inode, file_pointer);
}

static int custom_release(struct inode *inode,
                          struct file *file_pointer)
{
    file_pointer->private_data = NULL;

    printk(KERN_INFO "custom_release\n");

    return 0;
}

static long custom_ioctl(struct file *file_pointer,
                         unsigned int cmd,
                         unsigned long arg)
{
    return -ENOTTY;
}

/* CHANGED: proc_ops -> file_operations */
static const struct file_operations custom_fops = {
    .owner          = THIS_MODULE,
    .open           = custom_open,
    .read           = custom_read,
    .write          = custom_write,
    .release        = custom_release,
    .unlocked_ioctl = custom_ioctl,
};

static int vif_module_init(void)
{
    int ret;

    printk(KERN_INFO "vif_module_init: entry\n");

    /* Allocate major/minor numbers */
    ret = alloc_chrdev_region(&dev_num,
                              0,
                              1,
                              "vif_driver");
    if (ret)
        return ret;

    /* Initialize cdev */
    cdev_init(&my_device.cdev, &custom_fops);
    my_device.cdev.owner = THIS_MODULE;

    /* Register cdev */
    ret = cdev_add(&my_device.cdev,
                   dev_num,
                   1);
    if (ret)
    {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    /* Create /sys/class/vif_driver */
    vif_class = class_create("vif_driver");

    if (IS_ERR(vif_class))
    {
        cdev_del(&my_device.cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(vif_class);
    }

    /* Create /dev/vif_driver */
    {
        struct device *dev = device_create(vif_class, NULL, dev_num,
                                           NULL, "vif_driver");
        if (IS_ERR(dev)) {
            ret = PTR_ERR(dev);
            goto err_class;
        }
    }

    vinput_ringbuf_init(&input_rb);
    vinput_ringbuf_init(&output_rb);

    ret = vinput_virtual_init(&output_rb);
    if (ret)
        goto err_device;

    ret = vinput_input_init(&input_rb);
    if (ret)
        goto err_virtual;

    printk(KERN_INFO "Major = %d Minor = %d\n",
           MAJOR(dev_num),
           MINOR(dev_num));

    printk(KERN_INFO "vif_module_init: exit\n");

    return 0;

err_virtual:
    vinput_virtual_exit();
err_device:
    device_destroy(vif_class, dev_num);
err_class:
    class_destroy(vif_class);
    cdev_del(&my_device.cdev);
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void vif_module_exit(void)
{
    printk(KERN_INFO "vif_module_exit: entry\n");

    vinput_input_exit();
    vinput_virtual_exit();

    device_destroy(vif_class, dev_num);

    class_destroy(vif_class);

    cdev_del(&my_device.cdev);

    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "vif_module_exit: exit\n");
}

module_init(vif_module_init);
module_exit(vif_module_exit);