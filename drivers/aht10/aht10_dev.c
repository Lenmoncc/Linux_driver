#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/math64.h>

#define AHT10_ADDRESS 0x70
#define AHT10_CMD_INIT 0XE1
#define AHT10_CMD_TRIGGER 0XAC
#define AHT10_CMD_READ 0X71
#define AHT10_CMD_WRITE 0X70
#define AHT10_INIT_PRAM1 0X08
#define AHT10_INIT_PRAM2 0X00
#define AHT10_TRIGGER_PRAM1 0X33
#define AHT10_TRIGGER_PRAM2 0X00
#define AHT10_CMD_SOFTRESET 0XBA

#define AHT10_NAME "aht10"
#define AHT10_NUM 1

struct aht10_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    void *private_data; // I2C client
    int humidity; 
    int temperature; 
};

static struct aht10_dev aht10dev;
static uint8_t aht10_read_status(struct aht10_dev *dev);

static int aht10_read_regs(struct aht10_dev *dev, u8 reg, void *val, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    msg[0].addr = client->addr;
    msg[0].flags = 0; // 写操作
    msg[0].buf = &reg;
    msg[0].len = 1;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD; // 读操作
    msg[1].buf = val;
    msg[1].len = len;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret < 0) {
        printk(KERN_ERR "I2C read error\n");
        return ret;
    }
    return 0;
}

static int aht10_write_regs(struct aht10_dev *dev, u8 reg, u8 *buf, int len)
{
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    int ret;

    b[0] = reg;
    memcpy(&b[1], buf, len);

    msg.addr = client->addr;
    msg.flags = 0; // 写操作
    msg.buf = b;
    msg.len = len + 1;

    ret = i2c_transfer(client->adapter, &msg, 1);
    if (ret < 0) {
        printk(KERN_ERR "AHT10: aht10_write_regs failed with error code %d, addr: 0x%x, reg: 0x%x\n", ret, client->addr, reg);
    }
    return ret;
}


static uint8_t aht10_read_status(struct aht10_dev *dev)
{
    uint8_t status = 0;
    aht10_read_regs(dev, AHT10_CMD_READ, &status, 1);
    return status;
}

static int aht10_device_init(struct aht10_dev *dev)
{
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    uint8_t init_cmd[2] = {AHT10_INIT_PRAM1, AHT10_INIT_PRAM2};
    int ret;

    msleep(40);

    if ((aht10_read_status(dev) & 0x08)==0) {
        ret = aht10_write_regs(dev, AHT10_CMD_INIT, init_cmd, sizeof(init_cmd));
        if (ret < 0) {
            printk(KERN_ERR "AHT10: Init command failed\n");
            return ret;
        }
    }
    msleep(20);
    
    return 0;
}

static int aht10_get_data(struct aht10_dev *dev)
{
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    uint8_t trigger_cmd[2] = {AHT10_TRIGGER_PRAM1, AHT10_TRIGGER_PRAM2};
    uint8_t data[6] = {0};
    uint32_t humidity, temperature;
    int ret;

    ret = aht10_write_regs(dev, AHT10_CMD_TRIGGER, trigger_cmd, sizeof(trigger_cmd));
    if (ret < 0) {
        printk(KERN_ERR "AHT10: Trigger command failed\n");
        return ret;
    }

    msleep(75);

    if((aht10_read_status(dev) & 0x80) != 0){
        printk(KERN_ERR "AHT10: Busy\n");
        return -EBUSY;
    }

    ret = aht10_read_regs(dev, AHT10_CMD_READ, data, sizeof(data));
    if (ret < 0) {
        printk(KERN_ERR "AHT10: Read data failed\n");
        return ret;
    }

    
    printk(KERN_INFO "AHT10 Raw Data: [0]:0x%02x [1]:0x%02x [2]:0x%02x [3]:0x%02x [4]:0x%02x [5]:0x%02x\n",
           data[0], data[1], data[2], data[3], data[4], data[5]);

    humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (uint32_t)(data[3] >> 4);
    temperature = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | ((uint32_t)(data[5]));

    dev->humidity = div_u64((u64)humidity * 10000, 1 << 20);
    dev->temperature = div_u64((u64)temperature * 20000, 1 << 20) - 5000;

        
    printk(KERN_INFO "AHT10 Calculated: Humidity=%d.%02d%%, Temperature=%d.%02dC\n",
           dev->humidity/100, dev->humidity%100,
           dev->temperature/100, abs(dev->temperature%100));

    return 0;
}

static int aht10_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &aht10dev;
    aht10_device_init(&aht10dev);

    return 0;
}

static ssize_t aht10_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    struct aht10_dev *dev = filp->private_data;
    int data[2];
    long err = 0;
    int ret;

    if (cnt < sizeof(data)) {
        return -EINVAL; 
    }

    ret = aht10_get_data(dev);
    if (ret < 0) {
        return ret; 
    }

    data[0] = dev->humidity;
    data[1] = dev->temperature; 

    err = copy_to_user(buf, data, sizeof(data));
    if (err) {
        printk(KERN_ERR "Failed to copy data to user space\n");
        return -EFAULT; 
    }

    return sizeof(data);
}

static int aht10_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations aht10_ops = {
    .owner = THIS_MODULE,
    .open = aht10_open,
    .read = aht10_read,
    .release = aht10_release,
};

static int aht10_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    printk(KERN_INFO "AHT10 matched successfully\n");

    /* 1、构建设备号 */
    if (aht10dev.major) {
        aht10dev.devid = MKDEV(aht10dev.major, 0);
        register_chrdev_region(aht10dev.devid, AHT10_NUM, AHT10_NAME);
    } else {
        alloc_chrdev_region(&aht10dev.devid, 0, AHT10_NUM, AHT10_NAME);
        aht10dev.major = MAJOR(aht10dev.devid);
    }

    /* 2、注册设备 */
    cdev_init(&aht10dev.cdev, &aht10_ops);
    cdev_add(&aht10dev.cdev, aht10dev.devid, AHT10_NUM);

    /* 3、创建类 */
    aht10dev.class = class_create(THIS_MODULE, AHT10_NAME);
    if (IS_ERR(aht10dev.class)) {
        return PTR_ERR(aht10dev.class);
    }

    /* 4、创建设备 */
    aht10dev.device = device_create(aht10dev.class, NULL, aht10dev.devid, NULL, AHT10_NAME);
    if (IS_ERR(aht10dev.device)) {
        return PTR_ERR(aht10dev.device);
    }

    /* 5、保存 I2C 客户端 */
    aht10dev.private_data = client;

    return 0;
}

static int aht10_remove(struct i2c_client *client)
{
    /* 删除设备 */
    cdev_del(&aht10dev.cdev);
    unregister_chrdev_region(aht10dev.devid, AHT10_NUM);

    /* 注销掉类和设备 */
    device_destroy(aht10dev.class, aht10dev.devid);
    class_destroy(aht10dev.class);
    return 0;
}

static const struct i2c_device_id aht10_id[] = {
    {"myi2c,aht10", 0},
    {}
};

static const struct of_device_id aht10_of_match[] = {
    { .compatible = "myi2c,aht10" },
    {}
};

static struct i2c_driver aht10_driver = {
    .probe = aht10_probe,
    .remove = aht10_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "aht10",
        .of_match_table = aht10_of_match,
    },
    .id_table = aht10_id,
};


static int __init aht10_init(void)
{
    int ret = 0;

    ret = i2c_add_driver(&aht10_driver);
    if (ret) {
        printk(KERN_ERR "Failed to register aht10 driver\n");
        return ret;
    }
    printk(KERN_INFO "AHT10 driver registered\n");
    return 0;
}

static void __exit aht10_exit(void)
{
    i2c_del_driver(&aht10_driver);
    printk(KERN_INFO "AHT10 driver removed\n");
}

module_init(aht10_init);
module_exit(aht10_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("My Name");