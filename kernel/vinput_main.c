#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>      // ADDED
#include <linux/kernel.h>
#include <linux/errno.h>

#define BUFFER_SIZE 1024

struct my_device_data {
    struct cdev cdev;
    char kernel_buffer[BUFFER_SIZE];
    size_t buffer_length;
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raul");
MODULE_DESCRIPTION("Virtual Input Framework");

/* ADDED */
static dev_t dev_num;
static struct class *vif_class;

/* CHANGED: replaced proc node with actual device object */
static struct my_device_data my_device = {
    .buffer_length = 0,
};

static ssize_t custom_read(struct file *file_pointer,
                           char __user *user_space_buffer,
                           size_t count,
                           loff_t *offset)
{
    printk(KERN_INFO "custom_read: entry\n");

    struct my_device_data *my_data = file_pointer->private_data;
    size_t bytes_to_copy;

    if (*offset >= my_data->buffer_length)
        return 0;

    bytes_to_copy = min(count,
                        my_data->buffer_length - (size_t)*offset);

    if (copy_to_user(user_space_buffer,
                     my_data->kernel_buffer + *offset,
                     bytes_to_copy))
    {
        return -EFAULT;
    }

    *offset += bytes_to_copy;

    return bytes_to_copy;
}

static ssize_t custom_write(struct file *file_pointer,
                            const char __user *user_space_buffer,
                            size_t count,
                            loff_t *offset)
{
    printk(KERN_INFO "custom_write: entry\n");

    struct my_device_data *my_data = file_pointer->private_data;
    size_t bytes_to_copy;

    bytes_to_copy = min(count, (size_t)(BUFFER_SIZE - 1));

    if (copy_from_user(my_data->kernel_buffer,
                       user_space_buffer,
                       bytes_to_copy))
    {
        return -EFAULT;
    }

    my_data->kernel_buffer[bytes_to_copy] = '\0';
    my_data->buffer_length = bytes_to_copy;

    printk(KERN_INFO "custom_write received: %s\n",
           my_data->kernel_buffer);

    return bytes_to_copy;
}

static int custom_open(struct inode *inode,
                       struct file *file_pointer)
{
    struct my_device_data *my_data;

    my_data = container_of(inode->i_cdev,
                           struct my_device_data,
                           cdev);

    file_pointer->private_data = my_data;

    printk(KERN_INFO "custom_open\n");

    return 0;
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
    printk(KERN_INFO "custom_ioctl\n");

    return 0;
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
    device_create(vif_class,
                  NULL,
                  dev_num,
                  NULL,
                  "vif_driver");

    printk(KERN_INFO "Major = %d Minor = %d\n",
           MAJOR(dev_num),
           MINOR(dev_num));

    printk(KERN_INFO "vif_module_init: exit\n");

    return 0;
}

static void vif_module_exit(void)
{
    printk(KERN_INFO "vif_module_exit: entry\n");

    device_destroy(vif_class, dev_num);

    class_destroy(vif_class);

    cdev_del(&my_device.cdev);

    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "vif_module_exit: exit\n");
}

module_init(vif_module_init);
module_exit(vif_module_exit);