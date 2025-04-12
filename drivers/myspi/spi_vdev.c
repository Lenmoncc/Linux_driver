#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define DEVICE_NAME "virtual_spi_device"
#define CLASS_NAME "virtual_spi_class"
#define BUFFER_SIZE 128

struct virtual_spi_dev {
    struct spi_device *spi;
    struct cdev cdev;
    dev_t devno;
    struct class *class;
    struct device *device;
    char buffer[BUFFER_SIZE];
};

static struct virtual_spi_dev vspi_dev;

// 模拟SPI数据传输
static int virtual_spi_transfer(struct virtual_spi_dev *dev, const char *tx_buf, char *rx_buf, size_t len) {
    struct spi_transfer xfer = {
        .tx_buf = (void *)tx_buf,
        .rx_buf = (void *)rx_buf,
        .len = len,
        .bits_per_word = 8,
        .cs_change = 1, // 控制片选信号
    };

    struct spi_message msg;
    spi_message_init(&msg);
    spi_message_add_tail(&xfer, &msg);

    int ret = spi_sync(dev->spi, &msg);
    if (ret < 0) {
        dev_err(&dev->spi->dev, "SPI transfer failed: %d\n", ret);
        return ret;
    }
    return 0;
}

// 字符设备操作函数
static int virtual_spi_open(struct inode *inode, struct file *filp) {
    filp->private_data = &vspi_dev;
    return 0;
}

static ssize_t virtual_spi_read(struct file *filp, char __user *buf, size_t count, loff_t *off) {
    struct virtual_spi_dev *dev = filp->private_data;
    int ret;

    // 模拟读取操作
    memset(dev->buffer, 0, BUFFER_SIZE);
    ret = virtual_spi_transfer(dev, NULL, dev->buffer, count);
    if (ret < 0)
        return ret;

    ret = copy_to_user(buf, dev->buffer, count);
    return ret ? -EFAULT : count;
}

static ssize_t virtual_spi_write(struct file *filp, const char __user *buf, size_t count, loff_t *off) {
    struct virtual_spi_dev *dev = filp->private_data;
    int ret;

    // 模拟写入操作
    ret = copy_from_user(dev->buffer, buf, count);
    if (ret)
        return -EFAULT;

    return virtual_spi_transfer(dev, dev->buffer, NULL, count);
}

static const struct file_operations vspi_fops = {
    .owner = THIS_MODULE,
    .open = virtual_spi_open,
    .read = virtual_spi_read,
    .write = virtual_spi_write,
};

// SPI设备探测函数
static int virtual_spi_probe(struct spi_device *spi) {
    int ret;

    // 分配设备号
    ret = alloc_chrdev_region(&vspi_dev.devno, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    // 初始化字符设备
    cdev_init(&vspi_dev.cdev, &vspi_fops);
    ret = cdev_add(&vspi_dev.cdev, vspi_dev.devno, 1);
    if (ret < 0) {
        unregister_chrdev_region(vspi_dev.devno, 1);
        return ret;
    }

    // 创建设备节点
    vspi_dev.class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(vspi_dev.class)) {
        cdev_del(&vspi_dev.cdev);
        unregister_chrdev_region(vspi_dev.devno, 1);
        return PTR_ERR(vspi_dev.class);
    }

    vspi_dev.device = device_create(vspi_dev.class, NULL, vspi_dev.devno, NULL, DEVICE_NAME);
    if (IS_ERR(vspi_dev.device)) {
        ret = PTR_ERR(vspi_dev.device);
        printk(KERN_ERR "Failed to create device: %d\n", ret);
        device_destroy(vspi_dev.class, vspi_dev.devno);
        class_destroy(vspi_dev.class);
        cdev_del(&vspi_dev.cdev);
        unregister_chrdev_region(vspi_dev.devno, 1);
        return PTR_ERR(vspi_dev.device);
    }

    // 保存spi_device指针
    vspi_dev.spi = spi;

    printk(KERN_INFO "Virtual SPI device probed\n");
    return 0;
}

// SPI设备移除函数
static int virtual_spi_remove(struct spi_device *spi) {
    struct virtual_spi_dev *dev = &vspi_dev;

    device_destroy(dev->class, dev->devno);
    class_destroy(dev->class);
    cdev_del(&dev->cdev);
    unregister_chrdev_region(dev->devno, 1);

    printk(KERN_INFO "Virtual SPI device removed\n");
    return 0;
}

// 设备匹配表
static const struct of_device_id virtual_spi_of_match[] = {
    { .compatible = "virtual_spi_device" },
    { },
};
MODULE_DEVICE_TABLE(of, virtual_spi_of_match);

static const struct spi_device_id virtual_spi_id[] = {
    { "virtual_spi_device", 0 },
    { },
};
MODULE_DEVICE_TABLE(spi, virtual_spi_id);

// SPI驱动结构体
static struct spi_driver virtual_spi_driver = {
    .probe = virtual_spi_probe,
    .remove = virtual_spi_remove,
    .driver = {
        .name = "virtual_spi_device",
        .of_match_table = virtual_spi_of_match,
    },
    .id_table = virtual_spi_id,
};

// 模块初始化函数
static int __init virtual_spi_device_init(void) {

    int ret;

    printk(KERN_INFO "virtual_spi_device_init: Initializing SPI device driver\n");
    ret = spi_register_driver(&virtual_spi_driver);
    if (ret) {
        printk(KERN_ERR "virtual_spi_device_init: Failed to register SPI driver\n");
    }
    return ret;

    //return spi_register_driver(&virtual_spi_driver);
}

// 模块退出函数
static void __exit virtual_spi_device_exit(void) {
    printk(KERN_INFO "virtual_spi_device_exit: Exiting SPI device driver\n");
    spi_unregister_driver(&virtual_spi_driver);
}

module_init(virtual_spi_device_init);
module_exit(virtual_spi_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Virtual SPI Device Driver");