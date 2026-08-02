#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>

#define BUFFER_SIZE 1024

struct my_device_data {
    struct cdev cdev;
    char kernel_buffer[BUFFER_SIZE];
    size_t buffer_length = 0;

};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Raul");
MODULE_DESCRIPTION("Virtual Input Framework"); 

static struct proc_dir_entry *custom_proc_node;

static ssize_t custom_read(struct file* file_pointer,
                 char __user* user_space_buffer,
                 size_t count, 
                 loff_t* offset){
    printk("custom_read: entry\n");
    
    struct my_device_data *my_data = file->private_data;

    if (*offset >= buffer_length){
        return 0;
    }

    if (copy_to_user(user_space_buffer, my_data->kernel_buffer, my_data->buffer_length)){
        return -1;
    }

    *offset += my_data->buffer_length;

    return my_data->buffer_length;
};

static ssize_t custom_write(struct file* file_pointer, 
                 const char __user* user_space_buffer,
                 size_t count,
                 loff_t* offset){
    printk("custom_write: entry\n");
    
    struct my_device_data *my_data = file->private_data;

    size_t bytes_to_copy;
    bytes_to_copy = min(count, (size_t)(BUFFER_SIZE - 1));
    
    if (copy_from_user(my_data->kernel_buffer, user_space_buffer, bytes_to_copy)){
        return -1;
    }

    kernel_buffer[bytes_to_copy] = '\0';
    my_data->buffer_length = bytes_to_copy;

    printk("custom_write received: %s\n", kernel_buffer);

    return bytes_to_copy;
};

static int custom_open(struct inode* inode, struct file* file_pointer){
    struct my_device_data *my_data;
    
    my_data = container_of(inode->i_cdev, struct my_device_data, cdev);

    file->private_data = my_data;

    return 0;
};

static int custom_release(struct inode* inode, struct file* file_pointer){
    file->private_data = NULL;
    return 0;
};

static long	custom_ioctl(struct file* file_pointer, unsigned int cmd, unsigned long arg){
    return 0;
};

struct proc_ops custom_proc_ops = {
    .proc_read = custom_read,
    .proc_write = custom_write,
    .proc_open = custom_open,
    .proc_release = custom_release,
    .proc_ioctl = custom_ioctl
};

static int vif_module_init (void) {
    printk("vif_module_init: entry\n");

    custom_proc_node = proc_create("ldd_driver",
                                0666,
                                NULL,
                                &custom_proc_ops);
    
    printk("vif_module_init: exit\n");

    return 0;
}

static void vif_module_exit (void) {
    printk("vif_module_exit: entry\n");

    proc_remove(custom_proc_node);

    printk("vif_module_exit: exit\n");
}

module_init(vif_module_init);
module_exit(vif_module_exit);