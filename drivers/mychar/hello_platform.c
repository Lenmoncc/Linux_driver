#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DRV_NAME "hello_platform"   // platform驱动名称
#define DEVICE_NAME "hello-char"    //字符设备名称

static int major;   //存储动态分配的主设备号
static struct cdev hello_cdev;  //字符设备结构体
static struct class *hello_class;   //设备类指针

static const char msg[] = "Hello from kernel driver!\n";  
static int msg_len = sizeof(msg);

// 文件操作函数
static int hello_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "hello-char: Device opened\n");
    return 0;
}

static ssize_t hello_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    int bytes_left = 0;
    int bytes_read = 0;

    bytes_left = msg_len - *ppos;
    if (bytes_left == 0) {
        return 0;
    }

    bytes_read = min(count,(size_t)bytes_left);
    if (copy_to_user(buf, msg + *ppos, bytes_read)) {
        return -EFAULT;
    }

    *ppos += bytes_read;

    printk(KERN_INFO "hello-char: Read %d bytes\n",bytes_read);
    return bytes_read;
}

static ssize_t hello_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos) {
    printk(KERN_INFO "hello-char: Write done, %zu bytes received\n", count);
    return count;
}

static struct file_operations hello_fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .read = hello_read,
    .write = hello_write,
};

// Platform驱动Probe函数
static int hello_probe(struct platform_device *pdev) {
    dev_t devno;

    // 1. 动态分配设备号
    if (alloc_chrdev_region(&devno, 0, 1, DEVICE_NAME) < 0) {
        return -EBUSY;
    }
    major = MAJOR(devno);

    // 2. 初始化并添加字符设备
    cdev_init(&hello_cdev, &hello_fops);
    if (cdev_add(&hello_cdev, devno, 1) < 0) {
        unregister_chrdev_region(devno, 1);
        return -EFAULT;
    }

    // 3. 创建设备节点 /dev/hello-char
    hello_class = class_create(THIS_MODULE, "hello_class");
    if (IS_ERR(hello_class)) {
        cdev_del(&hello_cdev);
        unregister_chrdev_region(devno, 1);
        return PTR_ERR(hello_class);
    }
    device_create(hello_class, NULL, devno, NULL, DEVICE_NAME);

    printk(KERN_INFO "hello-char: Probe success, major=%d\n", major);
    return 0;
}

// Platform驱动Remove函数
static int hello_remove(struct platform_device *pdev) {
    dev_t devno = MKDEV(major, 0);

    // 清理资源
    device_destroy(hello_class, devno);
    class_destroy(hello_class);
    cdev_del(&hello_cdev);
    unregister_chrdev_region(devno, 1);

    printk(KERN_INFO "hello-char: Device removed\n");
    return 0;
}

// 设备树匹配表
static const struct of_device_id hello_of_match[] = {
    { .compatible = "myvendor,hello-char" }, // 必须与设备树中的compatible一致
    {},
};
MODULE_DEVICE_TABLE(of, hello_of_match);

// Platform驱动结构体
static struct platform_driver hello_driver = {
    .probe = hello_probe,
    .remove = hello_remove,
    .driver = {
        .name = DRV_NAME,
        .of_match_table = hello_of_match,
    },
};

// 模块初始化函数
static int __init hello_init(void)
{
    return platform_driver_register(&hello_driver);
}

// 模块退出函数
static void __exit hello_exit(void)
{
    platform_driver_unregister(&hello_driver);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhugaoting");