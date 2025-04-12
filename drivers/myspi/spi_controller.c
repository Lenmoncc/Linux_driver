#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/platform_device.h>  // 添加平台设备相关头文件
#include <linux/of_device.h> 


// 定义虚拟 SPI 主机数据结构体
struct virtual_spi_host {
    struct spi_master *master;
};

// 虚拟 SPI 主机传输函数
static int virtual_spi_transfer_one_message(struct spi_master *master, struct spi_message *mesg)
{
    struct spi_transfer *t;
    list_for_each_entry(t, &mesg->transfers, transfer_list) {
        if (t->tx_buf) {
            // 模拟发送数据
            printk(KERN_INFO "Virtual SPI: Sending %zu bytes\n", t->len);
            msleep(10); // 模拟传输延迟
        }
        if (t->rx_buf) {
            // 模拟接收数据
            printk(KERN_INFO "Virtual SPI: Receiving %zu bytes\n", t->len);
            memset(t->rx_buf, 0, t->len); // 填充虚拟数据
            msleep(10); // 模拟传输延迟
        }
    }
    mesg->status = 0;
    spi_finalize_current_message(master);
    return 0;
}

// 虚拟 SPI 主机探测函数
static int virtual_spi_probe(struct platform_device *pdev)
{
    struct virtual_spi_host *host;
    struct spi_master *master;
    int ret;

    // 分配虚拟 SPI 主机数据结构体
    host = devm_kzalloc(&pdev->dev, sizeof(*host), GFP_KERNEL);
    if (!host) {
        return -ENOMEM;
    }

    // 分配 spi_master 结构体
    master = spi_alloc_master(&pdev->dev, sizeof(*host));
    if (!master) {
        return -ENOMEM;
    }

    // 设置 spi_master 的成员
    master->bus_num = pdev->id;
    master->num_chipselect = 1; // 假设只有一个片选信号
    master->transfer_one_message = virtual_spi_transfer_one_message;

    // 保存主机数据指针
    host->master = master;
    platform_set_drvdata(pdev, host);

    // 注册 spi_master
    ret = spi_register_master(master);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register SPI master: %d\n", ret);
        return ret;
    }

    printk(KERN_INFO "Virtual SPI host registered\n");
    return 0;
}

// 虚拟 SPI 主机移除函数
static int virtual_spi_remove(struct platform_device *pdev)
{
    struct virtual_spi_host *host = platform_get_drvdata(pdev);

    // 注销 spi_master
    spi_unregister_master(host->master);
    printk(KERN_INFO "Virtual SPI host unregistered\n");
    return 0;
}

// 设备树匹配表
static const struct of_device_id virtual_spi_of_match[] = {
    { .compatible = "virtual,virtual-spi-host" },
    { /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, virtual_spi_of_match);

// 平台驱动结构体
static struct platform_driver virtual_spi_driver = {
    .driver = {
        .name = "virtual-spi-host",
        .of_match_table = virtual_spi_of_match,
        .owner = THIS_MODULE,
    },
    .probe = virtual_spi_probe,
    .remove = virtual_spi_remove,
};

// 模块初始化函数
static int __init virtual_spi_init(void)
{
    return platform_driver_register(&virtual_spi_driver);
}

// 模块退出函数
static void __exit virtual_spi_exit(void)
{
    platform_driver_unregister(&virtual_spi_driver);
}

module_init(virtual_spi_init);
module_exit(virtual_spi_exit);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Virtual SPI Host Driver");
MODULE_LICENSE("GPL");