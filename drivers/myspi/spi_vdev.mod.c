#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xfa985410, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x87a740ad, __VMLINUX_SYMBOL_STR(driver_unregister) },
	{ 0x34ce7e95, __VMLINUX_SYMBOL_STR(spi_register_driver) },
	{ 0x5d5890a1, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x3c8c7d13, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0xfeb25d8b, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xb6828306, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0x67c2fa54, __VMLINUX_SYMBOL_STR(__copy_to_user) },
	{ 0xfbc74f64, __VMLINUX_SYMBOL_STR(__copy_from_user) },
	{ 0x40a6d9af, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0xaf26be77, __VMLINUX_SYMBOL_STR(spi_sync) },
	{ 0xfa2a45e, __VMLINUX_SYMBOL_STR(__memzero) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0xb37da9a1, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x495be299, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x43c34b68, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("spi:virtual_spi_device");
MODULE_ALIAS("of:N*T*Cvirtual_spi_device*");

MODULE_INFO(srcversion, "B4D9B9124725BD8C18B3A77");
