#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xb92a4e79, "module_layout" },
	{ 0xfbc57363, "kobject_put" },
	{ 0x5cc2a511, "hrtimer_forward" },
	{ 0x695bf5e9, "hrtimer_cancel" },
	{ 0x47229b5c, "gpio_request" },
	{ 0xb163aa54, "gpio_to_desc" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x442b0b49, "kthread_create_on_node" },
	{ 0x20ae3040, "kobject_create_and_add" },
	{ 0xec523f88, "hrtimer_start_range_ns" },
	{ 0xc5850110, "printk" },
	{ 0xe90eba20, "kthread_stop" },
	{ 0x18be14d8, "sysfs_create_group" },
	{ 0xdd1d312b, "gpiod_direction_output_raw" },
	{ 0x8c8569cb, "kstrtoint" },
	{ 0x1000e51, "schedule" },
	{ 0xf1640135, "wake_up_process" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0xfe990052, "gpio_free" },
	{ 0xf1f548ec, "gpiod_set_raw_value" },
	{ 0xa362bf8f, "hrtimer_init" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "8B1D60BF5C36EF5EFA15983");
