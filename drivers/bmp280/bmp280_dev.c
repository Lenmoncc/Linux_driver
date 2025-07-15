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
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/math64.h>
#include "bmp280_reg.h"

#define BMP280_NUM	1
#define BMP280_NAME	"bmp280"

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
}bmp280_calib_data_t;


struct bmp280_dev {
	dev_t devid;				
	struct cdev cdev;			
	struct class *class;		
	struct device *device;		
	struct device_node	*nd; 	
	int major;					
	void *private_data;		
    bmp280_calib_data_t calib_data; 
    int32_t temperature;
    uint32_t pressure;	
};

static struct bmp280_dev bmp280dev;


static int bmp280_read_regs(struct bmp280_dev *dev, u8 reg, void *buf, int len)
{
    int ret = -1;
    unsigned char txdata[1];
    unsigned char * rxdata;
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;


    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    if(!t) {
        dev_err(&spi->dev, "read_regs: failed to allocate transfer struct\n");
        return -ENOMEM;
    }

    rxdata = kzalloc(sizeof(char) * len, GFP_KERNEL);
    if(!rxdata) {
        dev_err(&spi->dev, "read_regs: failed to allocate rx buffer\n");
        goto out1;
    }

    txdata[0] = reg | 0x80;
    t->tx_buf = txdata;
    t->rx_buf = rxdata;
    t->len = len+1;

    spi_message_init(&m);
    spi_message_add_tail(t, &m);

    ret = spi_sync(spi, &m);
    if(ret) {
        dev_err(&spi->dev, "read_regs: spi_sync failed: %d\n", ret);
        goto out2;
    }
   

    memcpy(buf , rxdata+1, len);

out2:
    kfree(rxdata);
out1:
    kfree(t);

    return ret;
}

static s32 bmp280_write_regs(struct bmp280_dev *dev, u8 reg, u8 *buf, u8 len)
{
    int ret = -1;
    unsigned char *txdata;
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;


    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    if(!t) {
        dev_err(&spi->dev, "write_regs: failed to allocate transfer struct\n");
        return -ENOMEM;
    }

    txdata = kzalloc(sizeof(char)+len, GFP_KERNEL);
    if(!txdata) {
        dev_err(&spi->dev, "write_regs: failed to allocate tx buffer\n");
        goto out1;
    }

    *txdata = reg & ~0x80;
    memcpy(txdata+1, buf, len);
    t->tx_buf = txdata;
    t->len = len+1;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);

    ret = spi_sync(spi, &m);
    if(ret) {
        dev_err(&spi->dev, "write_regs: spi_sync failed: %d\n", ret);
        goto out2;
    }

out2:
    kfree(txdata);
out1:
    kfree(t);

    return ret;
}

static int32_t bmp280_compensate_temp(struct bmp280_dev *dev, int32_t adc_temp, int32_t *t_fine)
{
	int32_t var1, var2, T;

	var1 = ((((adc_temp >> 3) - ((int32_t)dev->calib_data.dig_T1 << 1))) * 
		   ((int32_t)dev->calib_data.dig_T2)) >> 11;

	var2 = (((((adc_temp >> 4) - ((int32_t)dev->calib_data.dig_T1)) * 
			((adc_temp >> 4) - ((int32_t)dev->calib_data.dig_T1))) >> 12) * 
		   ((int32_t)dev->calib_data.dig_T3)) >> 14;

	*t_fine = var1 + var2;
	T = (*t_fine * 5 + 128) >> 8;
	return T;
}


static uint32_t bmp280_compensate_press(struct bmp280_dev *dev, int32_t adc_press, int32_t t_fine)
{
	int64_t var1, var2, p;
	uint64_t tmp;

    printk(KERN_INFO "BMP280 Pressure Calibration: t_fine = %d (0x%08x)\n", t_fine, t_fine);

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dev->calib_data.dig_P6;
    var2 = var2 + ((var1 * (int64_t)dev->calib_data.dig_P5) << 17);
    var2 = var2 + (((int64_t)dev->calib_data.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dev->calib_data.dig_P3) >> 8) +((var1 * (int64_t)dev->calib_data.dig_P2) << 12);
    var1 =(((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib_data.dig_P1) >> 33;

    if (var1 == 0)
    {
        return 0;
    }
    else
    {
        p = 1048576 - adc_press;
        //p = (((p << 31) - var2) * 3125) / var1;
		tmp = (uint64_t)(((p << 31) - var2) * 3125);
		p = do_div(tmp, var1);
        var1 = (((int64_t)dev->calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
        var2 = (((int64_t)dev->calib_data.dig_P8) * p) >> 19;
        p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib_data.dig_P7) << 4);
	}

	return (uint32_t)(p >> 8);
}

static int32_t bmp280_read_raw_temp(struct bmp280_dev *dev)
{
	uint8_t buf[3] = {0};
    int32_t raw_temp;
	bmp280_read_regs(dev, BMP_TEMP_MSB, buf, 3);

    raw_temp = (int32_t)(((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4));

	printk(KERN_INFO "BMP280 Raw Temp: [0]:0x%02x [1]:0x%02x [2]:0x%02x -> Raw: %d (0x%05x)\n",
           buf[0], buf[1], buf[2], raw_temp, raw_temp & 0xFFFFF);
	
		   return raw_temp;
}

static uint32_t bmp280_read_raw_press(struct bmp280_dev *dev)
{
	uint8_t buf[3] = {0};
    uint32_t raw_press;
    bmp280_read_regs(dev, BMP_PRESS_MSB, buf, 3);

    raw_press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    
	printk(KERN_INFO "BMP280 Raw Press: [0]:0x%02x [1]:0x%02x [2]:0x%02x -> Raw: %u (0x%05x)\n",
           buf[0], buf[1], buf[2], raw_press, raw_press);

	return raw_press;
}

static uint8_t bmp280_check_status(struct bmp280_dev *dev)
{
    uint8_t status = 0;
    uint8_t status_measure = 0;
    uint8_t status_imupdate = 0;
    bmp280_read_regs(dev, BMP_STATUS, &status, 1);
    status_measure = status & 0x08; 
    status_imupdate = status & 0x01;
    if ((status_measure!=0) && (status_imupdate!=0)) {   
        printk(KERN_INFO "BMP280 is busy\n");
        return 1;
    } else {
        printk(KERN_INFO "BMP280 is ready\n");
        return 0;
    }
}

static int bmp280_reset(struct bmp280_dev *dev)
{
    u8 reset_val = BMP_RESET_VALUE;
    int ret;

    ret = bmp280_write_regs(dev, BMP_RESET, &reset_val, 1);
    if (ret < 0) {
        dev_err(&((struct spi_device *)dev->private_data)->dev, "Reset failed: %d\n", ret);
        return ret;
    }

    msleep(10); 
    printk(KERN_INFO "BMP280 reset done\n");
    return 0;
}

static int bmp280_get_calib_data(struct bmp280_dev *dev)
{
    u8 calib_buf[24];
    int ret;

    ret = bmp280_read_regs(dev, BMP280_DIG_T1_LSB, calib_buf, 24);
    if (ret < 0) {
        dev_err(&((struct spi_device *)dev->private_data)->dev, "Calibration data read failed: %d\n", ret);
        return ret;
    }

	dev->calib_data.dig_T1 = (calib_buf[1] << 8) | calib_buf[0];
	dev->calib_data.dig_T2 = (calib_buf[3] << 8) | calib_buf[2];
	dev->calib_data.dig_T3 = (calib_buf[5] << 8) | calib_buf[4];
	dev->calib_data.dig_P1 = (calib_buf[7] << 8) | calib_buf[6];
	dev->calib_data.dig_P2 = (calib_buf[9] << 8) | calib_buf[8];
	dev->calib_data.dig_P3 = (calib_buf[11] << 8) | calib_buf[10];
	dev->calib_data.dig_P4 = (calib_buf[13] << 8) | calib_buf[12];
	dev->calib_data.dig_P5 = (calib_buf[15] << 8) | calib_buf[14];
	dev->calib_data.dig_P6 = (calib_buf[17] << 8) | calib_buf[16];
	dev->calib_data.dig_P7 = (calib_buf[19] << 8) | calib_buf[18];
	dev->calib_data.dig_P8 = (calib_buf[21] << 8) | calib_buf[20];
	dev->calib_data.dig_P9 = (calib_buf[23] << 8) | calib_buf[22];

    printk(KERN_INFO "BMP280 Calib: T1=%u, T2=%d, T3=%d\n", 
           dev->calib_data.dig_T1, dev->calib_data.dig_T2, dev->calib_data.dig_T3);
    printk(KERN_INFO "BMP280 Calib: P1=%u, P2=%d, P3=%d, P4=%d, P5=%d, P6=%d, P7=%d, P8=%d, P9=%d\n",
           dev->calib_data.dig_P1, dev->calib_data.dig_P2, dev->calib_data.dig_P3,
           dev->calib_data.dig_P4, dev->calib_data.dig_P5, dev->calib_data.dig_P6,
           dev->calib_data.dig_P7, dev->calib_data.dig_P8, dev->calib_data.dig_P9);

    return 0;
}


static int bmp280_device_init(struct bmp280_dev *dev)
{
	u8 id_value;
	int ret;
     
    u8 config_val = BMP_CONFIG_VALUE;      
    u8 ctrl_meas_val = BMP_CTRL_MEAS_VALUE; 


    struct spi_device *spi = dev->private_data;

	ret = bmp280_read_regs(dev, BMP_ID, &id_value, 1);    
	if (ret < 0) {
        dev_err(&spi->dev, "Read ID failed: %d\n", ret);
        return ret;
        }
	
	if (id_value != BMP_ID_VALUE) {
		dev_err(&spi->dev, "Invalid chip ID: 0x%x\n", id_value);
		return -ENODEV;
	}
	printk(KERN_INFO "BMP280 ID: 0x%02x\n", id_value);


    /* 获取修正参数 */
    bmp280_get_calib_data(dev);

    /* 复位 */
    bmp280_reset(dev);

    /* bmp280配置 */
	ret = bmp280_write_regs(dev, BMP_CTRL_MEAS, &ctrl_meas_val, 1);
	if (ret < 0)	
        return ret;

	ret = bmp280_write_regs(dev, BMP_CONFIG, &config_val, 1);
	if (ret < 0)
		return ret;

    printk(KERN_INFO "BMP280 initialized successfully\n");
	return 0;
}

static int bmp280_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &bmp280dev; 
	return 0;
}

static ssize_t bmp280_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
    struct bmp280_dev *dev = filp->private_data;
	int32_t adc_temp, adc_press, t_fine = 0;
	int ret;
	u8 data[30]; 
    u8 ctrl_val = BMP_CTRL_MEAS_VALUE; 

    ret = bmp280_write_regs(dev, BMP_CTRL_MEAS, &ctrl_val, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to start measurement\n");
        return ret;
    }

	while (bmp280_check_status(dev))
	mdelay(50);

	adc_temp = bmp280_read_raw_temp(dev);
	adc_press = bmp280_read_raw_press(dev);

	dev->temperature = bmp280_compensate_temp(dev, adc_temp, &t_fine);
	//dev->pressure = bmp280_compensate_press(dev, adc_press, t_fine);
    dev->pressure = bmp280_read_raw_press(dev);

	printk(KERN_INFO "BMP280 Compensated Temp: %d\n", dev->temperature);
    printk(KERN_INFO "BMP280 Compensated Press: %u\n", dev->pressure);

	data[0] = (dev->temperature >> 0) & 0xFF;
	data[1] = (dev->temperature >> 8) & 0xFF;
	data[2] = (dev->temperature >> 16) & 0xFF;
	data[3] = (dev->temperature >> 24) & 0xFF;
	data[4] = (dev->pressure >> 0) & 0xFF;
	data[5] = (dev->pressure >> 8) & 0xFF;
	data[6] = (dev->pressure >> 16) & 0xFF;
	data[7] = (dev->pressure >> 24) & 0xFF;
        
    // 填充t_fine（4字节，小端模式）
    data[8] = (t_fine >> 0) & 0xFF;
    data[9] = (t_fine >> 8) & 0xFF;
    data[10] = (t_fine >> 16) & 0xFF;
    data[11] = (t_fine >> 24) & 0xFF;

    // 填充压力校准参数（均为2字节，小端模式）
    // dig_P1: uint16_t
    data[12] = (dev->calib_data.dig_P1 >> 0) & 0xFF;
    data[13] = (dev->calib_data.dig_P1 >> 8) & 0xFF;
    // dig_P2: int16_t
    data[14] = (dev->calib_data.dig_P2 >> 0) & 0xFF;
    data[15] = (dev->calib_data.dig_P2 >> 8) & 0xFF;
    // dig_P3: int16_t
    data[16] = (dev->calib_data.dig_P3 >> 0) & 0xFF;
    data[17] = (dev->calib_data.dig_P3 >> 8) & 0xFF;
    // dig_P4: int16_t
    data[18] = (dev->calib_data.dig_P4 >> 0) & 0xFF;
    data[19] = (dev->calib_data.dig_P4 >> 8) & 0xFF;
    // dig_P5: int16_t
    data[20] = (dev->calib_data.dig_P5 >> 0) & 0xFF;
    data[21] = (dev->calib_data.dig_P5 >> 8) & 0xFF;
    // dig_P6: int16_t
    data[22] = (dev->calib_data.dig_P6 >> 0) & 0xFF;
    data[23] = (dev->calib_data.dig_P6 >> 8) & 0xFF;
    // dig_P7: int16_t
    data[24] = (dev->calib_data.dig_P7 >> 0) & 0xFF;
    data[25] = (dev->calib_data.dig_P7 >> 8) & 0xFF;
    // dig_P8: int16_t
    data[26] = (dev->calib_data.dig_P8 >> 0) & 0xFF;
    data[27] = (dev->calib_data.dig_P8 >> 8) & 0xFF;
    // dig_P9: int16_t
    data[28] = (dev->calib_data.dig_P9 >> 0) & 0xFF;
    data[29] = (dev->calib_data.dig_P9 >> 8) & 0xFF;


	ret = copy_to_user(buf, data, min(cnt, sizeof(data)));
	return ret ? -EFAULT : min(cnt, sizeof(data));
}

static int bmp280_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations bmp280_ops = {
	.owner = THIS_MODULE,
	.open = bmp280_open,
	.read = bmp280_read,
	.release = bmp280_release,
};


static int bmp280_probe(struct spi_device *spi)
{
    printk(KERN_INFO "BMP280 matched successfully\n");

	/* 1、构建设备号 */
    if (bmp280dev.major) {
        bmp280dev.devid = MKDEV(bmp280dev.major, 0);
        register_chrdev_region(bmp280dev.devid, BMP280_NUM, BMP280_NAME);
    } else {
        alloc_chrdev_region(&bmp280dev.devid, 0, BMP280_NUM, BMP280_NAME);
        bmp280dev.major = MAJOR(bmp280dev.devid);
    }

	/* 2、注册设备 */
    cdev_init(&bmp280dev.cdev, &bmp280_ops);
	cdev_add(&bmp280dev.cdev, bmp280dev.devid, BMP280_NUM);

	/* 3、创建类 */
	bmp280dev.class = class_create(THIS_MODULE, BMP280_NAME);
	if (IS_ERR(bmp280dev.class)) {
		return PTR_ERR(bmp280dev.class);
	}

	/* 4、创建设备 */
	bmp280dev.device = device_create(bmp280dev.class, NULL, bmp280dev.devid, NULL, BMP280_NAME);
	if (IS_ERR(bmp280dev.device)) {
		return PTR_ERR(bmp280dev.device);
	}

    /* 5、初始化配置 */
        spi->mode = SPI_MODE_0;	/*MODE0，CPOL=0，CPHA=0*/
        spi_setup(spi);
	bmp280dev.private_data = spi;

    /* 6、硬件初始化 */
    int ret = bmp280_device_init(&bmp280dev);
    if (ret < 0) {
        printk(KERN_ERR "BMP280 device initialization failed\n");
        return ret;
    }

    printk(KERN_INFO "BMP280 driver initialized successfully\n");

	return 0;
}


static int bmp280_remove(struct spi_device *spi)
{
    /* 删除设备 */
	cdev_del(&bmp280dev.cdev);
	unregister_chrdev_region(bmp280dev.devid, BMP280_NAME);

	/* 注销掉类和设备 */
	device_destroy(bmp280dev.class, bmp280dev.devid);
	class_destroy(bmp280dev.class);

    printk(KERN_INFO "bmp280 driver removed\n");
	return 0;
}


static const struct spi_device_id bmp280_id[] = {
	{"myspi,bmp280", 0},  
	{ }
};

static const struct of_device_id bmp280_of_match[] = {
	{ .compatible = "myspi,bmp280" },
	{ }
};

static struct spi_driver bmp280_driver = {
	.probe = bmp280_probe,
	.remove = bmp280_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "bmp280",
		   	.of_match_table = bmp280_of_match, 
		   },
	.id_table = bmp280_id,
};
		   

static int __init bmp280_init(void)
{
    int ret = 0;
	ret = spi_register_driver(&bmp280_driver);
    if (ret < 0) {
        printk(KERN_ERR "bmp280 driver register failed\n");
        return ret;
    }
    printk(KERN_INFO "bmp280 driver init\n");
    return 0;
}


static void __exit bmp280_exit(void)
{
	spi_unregister_driver(&bmp280_driver);
    printk(KERN_INFO "bmp280 driver exit\n");
}

module_init(bmp280_init);
module_exit(bmp280_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("My Name");
