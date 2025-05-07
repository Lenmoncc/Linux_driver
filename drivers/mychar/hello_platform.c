#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/proc_fs.h>    
#include <linux/seq_file.h>    
#include <linux/sysfs.h> 


#define DRV_NAME "hello_platform"   // platform驱动名称
#define DEVICE_NAME "hello-char"    //字符设备名称

static int major;   //存储动态分配的主设备号
static struct cdev hello_cdev;  //字符设备结构体
static struct class *hello_class;   //设备类指针
static int device_open_count = 0;

static int hello_proc_show(struct seq_file *m, void *v);

static int hello_proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "Hello Platform Driver Status:\n");
    seq_printf(m, " - Major Number: %d\n", major);
    seq_printf(m, " - Device Opens: %d\n", device_open_count);
    return 0;
}

// sysfs文件操作函数属性
static ssize_t debug_info_show(struct device *dev,
    struct device_attribute *attr, char *buf) {
return sprintf(buf, "Major: %d, Opens: %d\n", major, device_open_count);
}

static DEVICE_ATTR_RO(debug_info); // 定义只读属性

// 文件操作函数
static int hello_open(struct inode *inode, struct file *file) {
    device_open_count++; // 统计打开次数
    printk(KERN_INFO "hello-char: Device opened (total opens: %d)\n", device_open_count);
     return single_open(file, hello_proc_show, NULL);
}

static struct file_operations hello_fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
     .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

// Platform驱动Probe函数
static int hello_probe(struct platform_device *pdev) {
    dev_t devno;
    struct device *dev = &pdev->dev; 
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

    //创建proc文件
    proc_create("hello_proc", 0, NULL, &hello_fops);

    //创建sysfs属性文件
    if (device_create_file(dev, &dev_attr_debug_info)) {
        printk(KERN_ERR "Failed to create sysfs attribute\n");
    }

    printk(KERN_INFO "hello-char: Probe success, major=%d\n", major);
    return 0;
}

// Platform驱动Remove函数
static int hello_remove(struct platform_device *pdev) {
    dev_t devno = MKDEV(major, 0);

    // 清理proc文件
    remove_proc_entry("hello_proc", NULL);

    // 清理sysfs属性文件
    device_remove_file(&pdev->dev, &dev_attr_debug_info);

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
MODULE_DESCRIPTION("Platform Driver for Hello-Char Device");

