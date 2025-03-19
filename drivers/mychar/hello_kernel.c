#include <linux/init.h>   // 模块初始化和退出宏
#include <linux/module.h> // 内核模块基本功能
#include <linux/kernel.h> // 提供 printk 和日志级别宏

// 模块信息（可选，但建议添加）
MODULE_LICENSE("GPL");              // 许可证（必填）
MODULE_AUTHOR("Your Name");         // 作者
MODULE_DESCRIPTION("A simple kernel module with printk"); // 描述

// 模块初始化函数（加载时调用）
static int __init hello_init(void) {
    printk(KERN_INFO "Hello from kernel!\n");  // 输出到内核日志
    return 0; // 返回 0 表示成功
}

// 模块退出函数（卸载时调用）
static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye from kernel!\n");
}

// 注册初始化和退出函数
module_init(hello_init);
module_exit(hello_exit);