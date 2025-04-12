#include "linux/i2c.h"
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/timer.h>


static int major = 0;
static struct class *virtual_i2c_class;
struct i2c_client *virtual_client;


static ssize_t virtual_i2c_drv_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
    int ret = 0;

    struct i2c_msg msg[1];

    char *kbuf = kmalloc(size, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

        // 设置 I2C 消息
    msg[0].addr = virtual_client->addr;  // 从设备地址
    msg[0].flags = I2C_M_RD;             // 读标志
    msg[0].len = size;                   // 数据长度
    msg[0].buf = kbuf;                   // 数据缓冲区    

    ret = i2c_transfer(virtual_client->adapter, msg, 1);
    if (ret < 0) {
        printk(KERN_ERR "I2C transfer failed: %d\n", ret);
        kfree(kbuf);
        return ret;
    }

    if(copy_to_user(buf, msg[0].buf, size)) {
        printk(KERN_ERR "Failed to copy data to user\n");
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    //返回读取的数据长度
    return size;
}

static ssize_t virtual_i2c_drv_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int ret = 0;

    struct i2c_msg msg[1];
    char *kbuf = kmalloc(size, GFP_KERNEL);
	if (!kbuf)
		return -ENOMEM;
    
    if (copy_from_user(kbuf, buf, size)) {
        kfree(kbuf);
        return -EFAULT;
    }

    msg[0].addr = virtual_client->addr;  // 从设备地址
    msg[0].flags = 0;                    // 写标志
    msg[0].len = size;                   // 数据长度
    msg[0].buf = kbuf;                   // 数据缓冲区

    ret = i2c_transfer(virtual_client->adapter, msg, 1);
	if(ret < 0) {
        printk(KERN_ERR "I2C transfer failed: %d\n", ret);
        kfree(kbuf);
        return ret;
    }

    return size;
}


static struct file_operations virtual_i2c_drv_fops = {
	.owner = THIS_MODULE,
	.read = virtual_i2c_drv_read,
	.write = virtual_i2c_drv_write,
};


static int virtual_i2c_drv_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    printk(KERN_INFO "DRV_Entering probe\n");

    //保存client信息
    virtual_client = client;

    //注册设备
    major = register_chrdev(0, "virtual_i2c_drv", &virtual_i2c_drv_fops);
    if (major < 0) {
        printk(KERN_ERR "Failed to register char device: %d\n", major);
        return major;
    }

    //创建设备节点和设备类
    virtual_i2c_class = class_create(THIS_MODULE, "virtual_i2c_drv");
    if (IS_ERR(virtual_i2c_class)) {
        ret = PTR_ERR(virtual_i2c_class);
        unregister_chrdev(major, "virtual_i2c_drv");
        printk(KERN_ERR "Failed to create class: %d\n", ret);
        return ret;
    }
    device_create(virtual_i2c_class, NULL, MKDEV(major, 0), NULL, "virtual_i2c_drv");
    
    printk(KERN_INFO "I2C driver probed successfully\n");
	return 0;
}

static int virtual_i2c_drv_remove(struct i2c_client *client)
{
    //注销设备节点和设备类
    device_destroy(virtual_i2c_class, MKDEV(major, 0));
    class_destroy(virtual_i2c_class);
    unregister_chrdev(major, "virtual_i2c_drv");
    virtual_client = NULL;

    printk(KERN_INFO "I2C driver removed\n");
	return 0;
}

static const struct i2c_device_id virtual_i2c_drv_id[] = {
	{"virtual,virtual_i2c_drv",0},
	{ }
};

static const struct of_device_id virtual_i2c_drv_of_match[] = {
	{ .compatible = "virtual,virtual_i2c_drv"},
	{ },
};

static struct i2c_driver virtual_i2c_driver = {
	.driver = {
		.name = "virtual_i2c_drv",
		.owner = THIS_MODULE,
		.of_match_table = virtual_i2c_drv_of_match,
	},
	.probe = virtual_i2c_drv_probe,
	.remove = virtual_i2c_drv_remove,
    .id_table = virtual_i2c_drv_id,
};


static int __init virtual_i2c_drv_init(void)
{
    int ret;
    ret = i2c_add_driver(&virtual_i2c_driver);
    if (ret) {
        printk(KERN_ERR "Failed to register I2C driver: %d\n", ret);
        return ret;
    }
	
    printk(KERN_INFO "I2C driver registered successfully\n");
    return ret;
}

static void __exit virtual_i2c_drv_exit(void)
{
	i2c_del_driver(&virtual_i2c_driver);
    printk(KERN_INFO "I2C driver exit\n");
}


module_init(virtual_i2c_drv_init);
module_exit(virtual_i2c_drv_exit);

MODULE_AUTHOR("your_name");  
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C driver for my device");
