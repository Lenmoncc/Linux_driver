#include <linux/module.h>        
#include <linux/platform_device.h> 
#include <linux/serial_core.h>    
#include <linux/tty.h>           
#include <linux/tty_flip.h>      
#include <linux/device.h>         
#include <linux/slab.h>           
#include <linux/io.h>             
#include <linux/termios.h>        
#include <linux/errno.h>          
#include <linux/printk.h>         

#define BUFFER_SIZE 1024
#define UART_NR 1

//static struct  uart_port virtual_uart_port;
static struct uart_driver virtual_uart_driver;
static struct platform_driver virtual_uart_platform_driver;

struct virtual_uart{
    struct uart_port port;
};

static struct uart_driver virtual_uart_driver = {
    .owner = THIS_MODULE,
    .driver_name = "virtual_uart",
    .dev_name = "ttyTest",
    .major = 0,     //动态分配主设备号
    .minor = 0,     //次设备号从0开始动态分配
    .nr = UART_NR,
};

static unsigned int virtual_tx_empty(struct uart_port *port){
    printk(KERN_INFO "Check if the transmission buffer is empty\n");
    return TIOCSER_TEMT;
}

static void virtual_start_tx(struct uart_port *port){
    printk(KERN_INFO "Start transmission\n");
}

static void virtual_start_rx(struct uart_port *port){
    printk(KERN_INFO "Start receiving\n");
}

static int virtual_startup(struct uart_port *port){
    printk(KERN_INFO "Start UART\n");
    return 0;
}

static void virtual_shutdown(struct uart_port *port){
    printk(KERN_INFO "Shutdown UART\n");
}

static struct uart_ops virtual_uart_ops = {
    .tx_empty = virtual_tx_empty,    //检查发送缓冲区是否为空
    .throttle = virtual_start_rx,   //开始接收
    .start_tx = virtual_start_tx,    //开始发送
    .startup = virtual_startup,   //启动串口
    .shutdown = virtual_shutdown,   //关闭串口
};

static int virtual_uart_probe(struct platform_device *pdev)
{
    int ret;
    //struct virtual_uart_port *vuart;
    struct virtual_uart *vuart;

    vuart = devm_kzalloc(&pdev->dev, sizeof(*vuart), GFP_KERNEL);
    if(!vuart) return -ENOMEM;

    memset(&vuart->port, 0, sizeof(struct uart_port));
    vuart->port.uartclk = 1843200;  
    vuart->port.dev = &pdev->dev;   
    vuart->port.ops = &virtual_uart_ops;
    vuart->port.fifosize = BUFFER_SIZE;
    vuart->port.type = PORT_16550A;
    vuart->port.line = 0;
    vuart->port.iotype = UPIO_MEM;


    ret = uart_add_one_port(&virtual_uart_driver, &vuart->port);
    if(ret < 0) {
        dev_err(&pdev->dev, "uart_add_one_port failed\n");
        return ret;
    }

    //成功后才能设置
    platform_set_drvdata(pdev, vuart);
    return 0;
}

static int virtual_uart_remove(struct platform_device *pdev)
{
    struct virtual_uart *vuart = platform_get_drvdata(pdev);

    //检查空指针
    if(!vuart) {
        dev_err(&pdev->dev, "vuart NULL\n");
        return -EINVAL;
    }
    uart_remove_one_port(&virtual_uart_driver, &vuart->port);
    return 0;
}

static const struct of_device_id virtual_uart_match[] = {
    {.compatible = "virtual,virtual_uart"},
    {}
};

MODULE_DEVICE_TABLE(of, virtual_uart_match);

static struct platform_driver virtual_uart_platform_driver = {
    .probe = virtual_uart_probe,
    .remove = virtual_uart_remove,
    .driver = {
        .name = "virtual_uart",
        .of_match_table = virtual_uart_match,
    }
};

static int __init virtual_uart_init(void)
{
    int ret = uart_register_driver(&virtual_uart_driver);
    if(ret) {
        printk(KERN_ERR "uart_register_driver failed\n");
        return ret;
    }
    ret = platform_driver_register(&virtual_uart_platform_driver);
    if(ret){
        uart_unregister_driver(&virtual_uart_driver);
        printk(KERN_ERR "platform_driver_register failed\n");
        return ret;
    }

    printk(KERN_INFO "Virtual UART driver loaded successfully\n");
    return ret;
}

static void __exit virtual_uart_exit(void)
{
    uart_unregister_driver(&virtual_uart_driver);
    platform_driver_unregister(&virtual_uart_platform_driver);
    printk(KERN_INFO "Virtual UART driver unloaded successfully\n");
}

module_init(virtual_uart_init);
module_exit(virtual_uart_exit);

MODULE_AUTHOR("zhugaoting");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual UART Driver");



