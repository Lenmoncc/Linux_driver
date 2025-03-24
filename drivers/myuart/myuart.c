#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/of.h>          

#define DRIVER_NAME "simple_uart"
#define UART_MAJOR 200
#define UART_MINOR 0

static struct uart_port simple_port;
static struct uart_driver simple_uart_drv;
static struct platform_driver simple_platform_driver;

/* 硬件操作函数 */
static unsigned int simple_uart_tx_empty(struct uart_port *port) {
    return TIOCSER_TEMT;
}

static void simple_uart_set_termios(struct uart_port *port,
                                   struct ktermios *termios,
                                   struct ktermios *old) {
    unsigned int baud = uart_get_baud_rate(port, termios, old, 9600, 115200);
    printk(KERN_INFO "Baud rate set to: %u\n", baud);
}

static void simple_uart_start_tx(struct uart_port *port) {
    printk(KERN_INFO "Start transmission\n");
}

static void simple_uart_stop_tx(struct uart_port *port) {
    printk(KERN_INFO "Stop transmission\n");
}

static const struct uart_ops simple_uart_ops = {
    .tx_empty    = simple_uart_tx_empty,
    .set_termios = simple_uart_set_termios,
    .start_tx    = simple_uart_start_tx,
    .stop_tx     = simple_uart_stop_tx,
};

/* 平台设备探测函数 */
static int simple_uart_probe(struct platform_device *pdev) { 
    int ret;

    simple_uart_drv = (struct uart_driver) {
        .owner       = THIS_MODULE,
        .driver_name = DRIVER_NAME,
        .dev_name    = "ttySU",
        .major       = UART_MAJOR,
        .minor       = UART_MINOR,
        .nr          = 1,
    };

    ret = uart_register_driver(&simple_uart_drv);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register UART driver\n");
        return ret;
    }

    simple_port.iotype   = UPIO_MEM;
    simple_port.uartclk  = 1843200;
    simple_port.ops      = &simple_uart_ops;
    simple_port.line     = 0;

    ret = uart_add_one_port(&simple_uart_drv, &simple_port);
    if (ret) {
        uart_unregister_driver(&simple_uart_drv);
        return ret;
    }

    return 0;
}

/* 平台设备移除函数 */
static int simple_uart_remove(struct platform_device *pdev) {
    uart_remove_one_port(&simple_uart_drv, &simple_port);
    uart_unregister_driver(&simple_uart_drv);
    return 0;
}

/* 声明为静态常量匹配表 */
static const struct of_device_id simple_uart_match[] = {
    { .compatible = "mycompany,simple-uart" },
    {}
};
MODULE_DEVICE_TABLE(of, simple_uart_match);

/* 平台驱动定义 */
static struct platform_driver simple_platform_driver = {
    .probe  = simple_uart_probe,
    .remove = simple_uart_remove,
    .driver = {
        .name           = DRIVER_NAME,
        .of_match_table = simple_uart_match, 
    },
};

module_platform_driver(simple_platform_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple UART Driver");
MODULE_LICENSE("GPL");