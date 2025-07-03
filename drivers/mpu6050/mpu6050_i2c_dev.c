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
#include "mpu6050_reg.h"

#define MPU6050_CNT 1
#define MPU6050_NAME "mpu6050"

struct mpu6050_dev {
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    void *private_data;
    short gyro_x, gyro_y, gyro_z;
    short accel_x, accel_y, accel_z;
    short temp;
};

static struct mpu6050_dev mpu6050dev;

/*
 * @description : 从 MPU6050 读取多个寄存器数据
 * @param - dev:  MPU6050 设备
 * @param - reg:  要读取的寄存器首地址
 * @param - val:  读取到的数据
 * @param - len:  要读取的数据长度
 * @return      : 操作结果
 */
static int mpu6050_read_regs(struct mpu6050_dev *dev, u8 reg, void *val, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &reg;
    msg[0].len = 1;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = len;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret == 2) {
        ret = 0;
    } else {
        printk("i2c rd failed=%d reg=%06x len=%d\n", ret, reg, len);
        ret = -EREMOTEIO;
    }
    return ret;
}

/*
 * @description : 向 MPU6050 多个寄存器写入数据
 * @param - dev:  MPU6050 设备
 * @param - reg:  要写入的寄存器首地址
 * @param - val:  要写入的数据缓冲区
 * @param - len:  要写入的数据长度
 * @return      : 操作结果
 */
static s32 mpu6050_write_regs(struct mpu6050_dev *dev, u8 reg, u8 *buf, u8 len)
{
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    b[0] = reg;
    memcpy(&b[1], buf, len);

    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = b;
    msg.len = len + 1;

    return i2c_transfer(client->adapter, &msg, 1);
}

/*
 * @description : 读取 MPU6050 指定寄存器值，读取一个寄存器
 * @param - dev:  MPU6050 设备
 * @param - reg:  要读取的寄存器
 * @return      : 读取到的寄存器值
 */
static unsigned char mpu6050_read_reg(struct mpu6050_dev *dev, u8 reg)
{
    u8 data = 0;
    mpu6050_read_regs(dev, reg, &data, 1);
    return data;
}

/*
 * @description : 向 MPU6050 指定寄存器写入指定的值，写一个寄存器
 * @param - dev:  MPU6050 设备
 * @param - reg:  要写的寄存器
 * @param - data: 要写入的值
 * @return      : 无
 */
static void mpu6050_write_reg(struct mpu6050_dev *dev, u8 reg, u8 data)
{
    u8 buf = data;
    mpu6050_write_regs(dev, reg, &buf, 1);
}

/*
 * @description : 读取 MPU6050 的数据，读取原始数据，包括陀螺仪、加速度计和温度
 * @param - dev:  MPU6050 设备
 * @return      : 无
 */
void mpu6050_readdata(struct mpu6050_dev *dev)
{
    unsigned char buf[14];
    mpu6050_read_regs(dev, MPU6050_ACCEL_XOUT_H, buf, 14);

    dev->accel_x = ((short)buf[0] << 8) | buf[1];
    dev->accel_y = ((short)buf[2] << 8) | buf[3];
    dev->accel_z = ((short)buf[4] << 8) | buf[5];

    dev->temp = ((short)buf[6] << 8) | buf[7];

    dev->gyro_x = ((short)buf[8] << 8) | buf[9];
    dev->gyro_y = ((short)buf[10] << 8) | buf[11];
    dev->gyro_z = ((short)buf[12] << 8) | buf[13];
}

/*
 * @description : 打开设备
 * @param - inode : 传递给驱动的 inode
 * @param - filp  : 设备文件，file 结构体有个叫做 private_data 的成员变量
 *                  一般在 open 的时候将 private_data 指向设备结构体。
 * @return        : 0 成功;其他 失败
 */
static int mpu6050_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &mpu6050dev;

    /* 初始化 MPU6050 */
    mpu6050_write_reg(&mpu6050dev, MPU6050_PWR_MGMT_1, 0x00); // 唤醒 MPU6050
    return 0;
}

/*
 * @description : 从设备读取数据 
 * @param - filp  : 要打开的设备文件(文件描述符)
 * @param - buf   : 返回给用户空间的数据缓冲区
 * @param - cnt   : 要读取的数据长度
 * @param - offt  : 相对于文件首地址的偏移
 * @return        : 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t mpu6050_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    short data[7];
    long err = 0;

    struct mpu6050_dev *dev = (struct mpu6050_dev *)filp->private_data;

    mpu6050_readdata(dev);

    data[0] = dev->accel_x;
    data[1] = dev->accel_y;
    data[2] = dev->accel_z;
    data[3] = dev->temp;
    data[4] = dev->gyro_x;
    data[5] = dev->gyro_y;
    data[6] = dev->gyro_z;

    err = copy_to_user(buf, data, sizeof(data));
    return err ? -EFAULT : sizeof(data);
}

/*
 * @description : 关闭/释放设备
 * @param - filp  : 要关闭的设备文件(文件描述符)
 * @return        : 0 成功;其他 失败
 */
static int mpu6050_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/* MPU6050 操作函数 */
static const struct file_operations mpu6050_ops = {
    .owner = THIS_MODULE,
    .open = mpu6050_open,
    .read = mpu6050_read,
    .release = mpu6050_release,
};

/*
 * @description : i2c 驱动的 probe 函数，当驱动与
 *                设备匹配以后此函数就会执行
 * @param - client : i2c 设备
 * @param - id     : i2c 设备 ID
 * @return         : 0，成功;其他负值,失败
 */
static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    printk(KERN_INFO "MPU6050 driver matched with device successfully!\n");

    /* 1、构建设备号 */
    if (mpu6050dev.major) {
        mpu6050dev.devid = MKDEV(mpu6050dev.major, 0);
        register_chrdev_region(mpu6050dev.devid, MPU6050_CNT, MPU6050_NAME);
    } else {
        alloc_chrdev_region(&mpu6050dev.devid, 0, MPU6050_CNT, MPU6050_NAME);
        mpu6050dev.major = MAJOR(mpu6050dev.devid);
    }

    /* 2、注册设备 */
    cdev_init(&mpu6050dev.cdev, &mpu6050_ops);
    cdev_add(&mpu6050dev.cdev, mpu6050dev.devid, MPU6050_CNT);

    /* 3、创建类 */
    mpu6050dev.class = class_create(THIS_MODULE, MPU6050_NAME);
    if (IS_ERR(mpu6050dev.class)) {
        return PTR_ERR(mpu6050dev.class);
    }

    /* 4、创建设备 */
    mpu6050dev.device = device_create(mpu6050dev.class, NULL, mpu6050dev.devid, NULL, MPU6050_NAME);
    if (IS_ERR(mpu6050dev.device)) {
        return PTR_ERR(mpu6050dev.device);
    }

    mpu6050dev.private_data = client;

    return 0;
}

/*
 * @description : i2c 驱动的 remove 函数，移除 i2c 驱动的时候此函数会执行
 * @param - client : i2c 设备
 * @return         : 0，成功;其他负值,失败
 */
static int mpu6050_remove(struct i2c_client *client)
{
    /* 删除设备 */
    cdev_del(&mpu6050dev.cdev);
    unregister_chrdev_region(mpu6050dev.devid, MPU6050_CNT);

    /* 注销掉类和设备 */
    device_destroy(mpu6050dev.class, mpu6050dev.devid);
    class_destroy(mpu6050dev.class);

    printk(KERN_INFO "MPU6050 driver removed successfully!\n");
    return 0;
}

/* 传统匹配方式 ID 列表 */
static const struct i2c_device_id mpu6050_id[] = {
    {"invensense,mpu6050", 0},
    {}
};

/* 设备树匹配列表 */
static const struct of_device_id mpu6050_of_match[] = {
    { .compatible = "invensense,mpu6050" },
    { /* Sentinel */ }
};

/* i2c 驱动结构体 */
static struct i2c_driver mpu6050_driver = {
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "mpu6050",
        .of_match_table = mpu6050_of_match,
    },
    .id_table = mpu6050_id,
};

/*
 * @description : 驱动入口函数
 * @param       : 无
 * @return      : 无
 */
static int __init mpu6050_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&mpu6050_driver);
    if (ret == 0) {
        printk(KERN_INFO "MPU6050 driver loaded successfully!\n");
    } else {
        printk(KERN_ERR "Failed to load MPU6050 driver!\n");
    }
    return ret;
}

/*
 * @description : 驱动出口函数
 * @param       : 无
 * @return      : 无
 */
static void __exit mpu6050_exit(void)
{
    i2c_del_driver(&mpu6050_driver);
    printk(KERN_INFO "MPU6050 driver unloaded successfully!\n");
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("My Name");