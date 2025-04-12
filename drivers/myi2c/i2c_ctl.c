#include <linux/completion.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c-algo-bit.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>

static struct i2c_adapter *my_adapter;

static int virtual_i2c_master_xfer(struct i2c_adapter *i2c_adap,
		    struct i2c_msg msgs[], int num)
{
	int i,ret = 0;

	//检查参数
	if(!i2c_adap || !msgs || num <= 0){
		printk(KERN_ERR "Invalid parameters\n");
		return -EINVAL;
	}

	for(i=0; i<num; i++){
		//数据传输
		printk(KERN_INFO "virtual_i2c_master_xfer: processing message\n");
	}

	printk(KERN_INFO "virtual_i2c_master_xfer: %d messages processed\n", num);
	return num;
}

const struct i2c_algorithm virtual_i2c_algo = {
	.master_xfer   = virtual_i2c_master_xfer,
	
};


static int virtual_i2c_probe(struct platform_device *pdev)
{
	int ret;
	printk(KERN_INFO "Entering probe\n");

    //为i2c_adapter分配内存
	my_adapter = kzalloc(sizeof(*my_adapter), GFP_KERNEL);
	if(!my_adapter){
		printk(KERN_ERR "Failed to allocate memory\n");
		return -ENOMEM;
	}

	//初始化i2c_adapter结构体
	my_adapter->owner = THIS_MODULE;
	my_adapter->class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
	my_adapter->nr = 4;
	my_adapter->algo = &virtual_i2c_algo;
	strcpy(my_adapter->name, "virtual_i2c_adapter");
	//my_adapter->name = "virtual_i2c_adapter";
	printk(KERN_INFO "Adapter registered as i2c-%d\n", my_adapter->nr);

	//注册i2c_adapter
	ret = i2c_add_adapter(my_adapter);
	if(ret<0){
		printk(KERN_ERR "i2c_add_adapter failed: %d\n", ret);
		dev_err(&pdev->dev,"failed to register i2c_adapter: %d\n", ret);
		kfree(my_adapter);
		my_adapter = NULL;
		return ret;
	}

	printk(KERN_INFO,"i2c probed successfully\n");

	return 0;
}


static int virtual_i2c_remove(struct platform_device *pdev)
{
	i2c_del_adapter(my_adapter);
	kfree(my_adapter);
	my_adapter = NULL;
	printk(KERN_INFO "I2C driver removed\n");
	return 0;
}


static const struct i2c_device_id virtual_i2c_id[] = {
	{"virtual,virtual_i2c_ctl",0},
	{ /* sentinel */ }
};

static const struct of_device_id virtual_i2c_of_match[] = {
	{ .compatible = "virtual,virtual_i2c_ctl"},
	{ /* sentinel */ }
};

static struct platform_driver virtual_i2c_driver = {
	.driver		= {
		.owner	= THIS_MODULE,  
		.name	= "virtual_i2c_ctl",
		.of_match_table	= virtual_i2c_of_match,  
	},
	.probe		= virtual_i2c_probe,  
	.remove		= virtual_i2c_remove,  
	.id_table	= virtual_i2c_id,  
};


static int __init virtual_i2c_init(void)
{
	int ret;

	ret = platform_driver_register(&virtual_i2c_driver);
	if (ret){
		printk(KERN_INFO "Fail to Initialize I2C\n");
		return ret;
	}

	printk(KERN_INFO "Success Initializing I2C\n");
	return ret;
}

static void __exit virtual_i2c_exit(void)
{
	platform_driver_unregister(&virtual_i2c_driver);
	printk(KERN_INFO "I2C driver exit\n");
}

module_init(virtual_i2c_init);
module_exit(virtual_i2c_exit);

MODULE_AUTHOR("your_name");  
MODULE_DESCRIPTION("Virtual I2C bus driver");  
MODULE_LICENSE("GPL");



