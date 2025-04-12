#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define IMX6UL_SPI_BASE 0x02020000  // 这里的地址需要根据实际情况来修改
#define IMX6UL_SPI_IRQ  25          // 中断号需要根据硬件配置修改

struct imx6ul_spi {
    void __iomem *base;
    struct spi_master *master;
    int irq;
};

static irqreturn_t imx6ul_spi_irq_handler(int irq, void *dev_id)
{
    struct imx6ul_spi *spi = dev_id;
    
    // 中断处理逻辑

    return IRQ_HANDLED;
}


static int imx6ul_spi_probe(struct platform_device *pdev)
{
    struct imx6ul_spi *spi;
    struct resource *res;
    int ret;

    spi = devm_kzalloc(&pdev->dev, sizeof(*spi), GFP_KERNEL);
    if (!spi)
        return -ENOMEM;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    spi->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(spi->base))
        return PTR_ERR(spi->base);

    spi->irq = platform_get_irq(pdev, 0);
    if (spi->irq < 0)
        return spi->irq;

    ret = devm_request_irq(&pdev->dev, spi->irq, imx6ul_spi_irq_handler,
                           0, "imx6ul-spi", spi);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ\n");
        return ret;
    }

    spi->master = spi_alloc_master(&pdev->dev, sizeof(*spi));
    if (!spi->master)
        return -ENOMEM;

    spi->master->bus_num = pdev->id;
    spi->master->num_chipselect = 1;  // SPI 的 chipselect 数量
    spi->master->max_speed_hz = 50000000;
    //spi->master->mode = SPI_MODE_0;  // 设置 SPI 模式

    ret = spi_register_master(spi->master);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register SPI master\n");
        spi_master_put(spi->master);
        return ret;
    }

    platform_set_drvdata(pdev, spi);

    return 0;
}


static int imx6ul_spi_remove(struct platform_device *pdev)
{
    struct imx6ul_spi *spi = platform_get_drvdata(pdev);

    spi_unregister_master(spi->master);
    return 0;
}


static int imx6ul_spi_transfer_one(struct spi_master *master, struct spi_device *spi,
                                   struct spi_transfer *xfer)
{
    struct imx6ul_spi *spi_priv = spi_master_get_devdata(master);
    
    // 配置 SPI 控制器，传输数据
    // 读取和写入 SPI 控制寄存器，触发数据传输
    // 等待传输完成并处理接收数据

    return 0;
}


static const struct of_device_id imx6ul_spi_of_match[] = {
    { .compatible = "fsl,imx6ul-spi", },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, imx6ul_spi_of_match);

static struct platform_driver imx6ul_spi_driver = {
    .probe = imx6ul_spi_probe,
    .remove = imx6ul_spi_remove,
    .driver = {
        .name = "imx6ul-spi",
        .of_match_table = imx6ul_spi_of_match,
    },
};

module_platform_driver(imx6ul_spi_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("i.MX6UL SPI Driver");
MODULE_LICENSE("GPL");







