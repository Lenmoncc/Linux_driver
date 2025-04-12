#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>        
#include <linux/slab.h>        
#include <linux/uaccess.h>     
#include <linux/spi/spi.h>

// 全局静态的虚拟SPI设备结构体
static struct virtual_spi_device vdev;

#define DEVICE_NAME "myspidev"
#define MAX_BUF_SIZE 1024

#define STATIC_MAJOR 240
#define STATIC_MINOR 0

/* 设备结构体 */
struct virtual_spi_device {
    struct device *device;
    struct cdev *cdev;
    struct class *class;
    struct spi_device *spi;
    u8 rx_buf[MAX_BUF_SIZE];
    u8 tx_buf[MAX_BUF_SIZE];
    dev_t devt;
    int major;
};

//struct virtual_spi_device *my_spi_device;

static int my_spi_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    printk(KERN_INFO "SPI device opened\n");
    return ret;
}

//static my_spi_read(struct file *filp, char __user *buf, size_t, loff_t *pos)
static ssize_t my_spi_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;
    printk(KERN_INFO "SPI device read\n");
    return ret;
}

static int my_spi_release(struct inode *inode, struct file *filp)
{
    int ret = 0;
    printk(KERN_INFO "SPI device released\n");
    return ret;
}

static const struct file_operations my_spi_fops = {
    .owner = THIS_MODULE,
    .open = my_spi_open,
    .read = my_spi_read,
    .release = my_spi_release,
};

static int my_spi_driver_probe(struct spi_device *spi)
{
int ret;
#if 0
    /* 动态分配设备号 */
    ret = alloc_chrdev_region(&vdev.devt, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate chrdev region\n");
        return ret;
    }
    vdev.major = MAJOR(vdev.devt);

    vdev.cdev = cdev_alloc();
    if (!vdev.cdev) {
        printk(KERN_ERR "Failed to allocate cdev\n");
        ret = -ENOMEM;
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }
    printk(KERN_INFO "Allocated device number: major=%d, minor=%d\n", MAJOR(vdev.devt), MINOR(vdev.devt));

    /* 初始化字符设备 */
    cdev_init(vdev.cdev, &my_spi_fops);
    vdev.cdev->owner = THIS_MODULE;
    ret = cdev_add(vdev.cdev, vdev.devt, 1);
    if (ret) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }

    /* 创建设备类 */
    vdev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(vdev.class)) {
        printk(KERN_ERR "Failed to create class\n");
        ret = PTR_ERR(vdev.class);
        cdev_del(vdev.cdev);
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }

    /* 创建设备节点 */
    vdev.device = device_create(vdev.class, NULL, vdev.devt, NULL, DEVICE_NAME);
    if (IS_ERR(vdev.device)) {
        ret = PTR_ERR(vdev.device);
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(vdev.class);
        cdev_del(vdev.cdev);
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }
 #endif
    /* 关联SPI设备 */
    vdev.spi = spi;
    spi_set_drvdata(spi, &vdev);

    printk(KERN_INFO "SPI device probed\n");
    return 0;

}

static int	my_spi_driver_remove(struct spi_device *spi)
{
    int ret = 0;
    
    device_destroy(vdev.class, vdev.devt);
    class_destroy(vdev.class);
    cdev_del(vdev.cdev);
    unregister_chrdev_region(vdev.devt, 1);
    
    printk(KERN_INFO "SPI device removed\n");
    return ret;
}


/* 传统匹配 */
static const struct of_device_id my_spi_driver_dt_ids[] = {
	{ .compatible = "my_spi_driver",0 },
	{},
};
MODULE_DEVICE_TABLE(of, my_spi_driver_dt_ids);

#if 0
static const struct of_device_id my_spi_driver_of_match[] = {
    { .compatible = "virtual,my_spi_driver"},
    { /* sentinel */ }
};
#endif


static struct spi_driver my_spi_driver = {
	.driver = {
		.name =		"my_spi_driver",
		.owner =	THIS_MODULE,
		//.of_match_table = my_spi_driver_of_match,
	},
	.probe =	my_spi_driver_probe,
	.remove =	my_spi_driver_remove,
    .id_table =	my_spi_driver_dt_ids,
};

/* 驱动入口函数 */
static int __init myspi_driver_init(void)
{
    int ret;

    // 使用静态设备号
    vdev.devt = MKDEV(STATIC_MAJOR, STATIC_MINOR);
    ret = register_chrdev_region(vdev.devt, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "Failed to register chrdev region\n");
        return ret;
    }
    vdev.major = MAJOR(vdev.devt);

    printk(KERN_INFO "Registered device number: major=%d, minor=%d\n", MAJOR(vdev.devt), MINOR(vdev.devt));

    vdev.cdev = cdev_alloc();
    if (!vdev.cdev) {
        printk(KERN_ERR "Failed to allocate cdev\n");
        ret = -ENOMEM;
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }
    printk(KERN_INFO "Allocated device number: major=%d, minor=%d\n", MAJOR(vdev.devt), MINOR(vdev.devt));

    /* 初始化字符设备 */
    cdev_init(vdev.cdev, &my_spi_fops);
    vdev.cdev->owner = THIS_MODULE;
    ret = cdev_add(vdev.cdev, vdev.devt, 1);
    if (ret) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }

    /* 创建设备类 */
    vdev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(vdev.class)) {
        printk(KERN_ERR "Failed to create class\n");
        ret = PTR_ERR(vdev.class);
        cdev_del(vdev.cdev);
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }

    /* 创建设备节点 */
    vdev.device = device_create(vdev.class, NULL, vdev.devt, NULL, DEVICE_NAME);
    if (IS_ERR(vdev.device)) {
        ret = PTR_ERR(vdev.device);
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(vdev.class);
        cdev_del(vdev.cdev);
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }

    ret = spi_register_driver(&my_spi_driver);
    if (ret) {
        printk(KERN_ERR "Failed to register SPI driver\n");
        device_destroy(vdev.class, vdev.devt);
        class_destroy(vdev.class);
        cdev_del(vdev.cdev);
        unregister_chrdev_region(vdev.devt, 1);
        return ret;
    }
    printk(KERN_INFO "SPI driver registered\n");
    return ret;
}

/* 驱动出口函数 */
static void __exit myspi_driver_exit(void)
{
    spi_unregister_driver(&my_spi_driver);
    device_destroy(vdev.class, vdev.devt);
    class_destroy(vdev.class);
    cdev_del(vdev.cdev);
    unregister_chrdev_region(vdev.devt, 1);
    printk(KERN_INFO "SPI driver unregistered\n");
}

module_init(myspi_driver_init);
module_exit(myspi_driver_exit);

MODULE_AUTHOR("Your Name");
MODULE_LICENSE("GPL");

