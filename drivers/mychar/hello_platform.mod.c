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
	{ 0xc7afa9f7, __VMLINUX_SYMBOL_STR(single_release) },
	{ 0xd6060075, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0x2000cf79, __VMLINUX_SYMBOL_STR(seq_lseek) },
	{ 0x51204841, __VMLINUX_SYMBOL_STR(platform_driver_unregister) },
	{ 0xda113ac9, __VMLINUX_SYMBOL_STR(__platform_driver_register) },
	{ 0x7e7cfb2e, __VMLINUX_SYMBOL_STR(device_create_file) },
	{ 0xa8e20413, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0x5d5890a1, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x3c8c7d13, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0xfeb25d8b, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xb6828306, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0xd21e7ed9, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0x23e615a2, __VMLINUX_SYMBOL_STR(single_open) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0xb37da9a1, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x495be299, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x43c34b68, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0xc1c23d4b, __VMLINUX_SYMBOL_STR(device_remove_file) },
	{ 0x84c19f36, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("of:N*T*Cmyvendor,hello-char*");

MODULE_INFO(srcversion, "684556FF512C0AD8749FC75");
