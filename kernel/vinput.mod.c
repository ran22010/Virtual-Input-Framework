#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x5e505530, "kthread_should_stop" },
	{ 0x68a1b6c6, "__wake_up" },
	{ 0xc0481510, "input_register_handle" },
	{ 0x11f4259a, "_raw_spin_lock_irqsave" },
	{ 0xd272d446, "__fentry__" },
	{ 0xa3a19834, "wake_up_process" },
	{ 0xb524cc7d, "input_free_device" },
	{ 0xd272d446, "dump_stack" },
	{ 0xe8213e80, "_printk" },
	{ 0xbd03ed67, "__ref_stack_chk_guard" },
	{ 0x0e675b65, "___ratelimit" },
	{ 0xd272d446, "schedule" },
	{ 0x69b2b2dd, "input_register_device" },
	{ 0xd272d446, "__stack_chk_fail" },
	{ 0x90a48d82, "__ubsan_handle_out_of_bounds" },
	{ 0x45d300cd, "cdev_add" },
	{ 0x7a5ffe84, "init_wait_entry" },
	{ 0x85be535a, "device_create" },
	{ 0xc0481510, "input_open_device" },
	{ 0xfad798b2, "class_create" },
	{ 0xbd03ed67, "random_kmalloc_seed" },
	{ 0x2794f3c0, "kthread_stop" },
	{ 0x7057d579, "mutex_lock_interruptible" },
	{ 0x9b15d336, "input_close_device" },
	{ 0x444885a7, "_raw_spin_unlock_irqrestore" },
	{ 0x463753d3, "input_unregister_handler" },
	{ 0xd272d446, "__x86_return_thunk" },
	{ 0x092a35a2, "_copy_to_user" },
	{ 0xe804603d, "__init_waitqueue_head" },
	{ 0xfcf8942c, "input_event" },
	{ 0x888b8f57, "strcmp" },
	{ 0x59cf8621, "nonseekable_open" },
	{ 0xca31368d, "kthread_create_on_node" },
	{ 0x35e43a03, "input_register_handler" },
	{ 0x0bc5fb0d, "unregister_chrdev_region" },
	{ 0x9aa6980d, "mutex_unlock" },
	{ 0x0f889972, "device_destroy" },
	{ 0x30eb81ed, "__dynamic_pr_debug" },
	{ 0xce56ea17, "__kmalloc_cache_noprof" },
	{ 0x97acb853, "ktime_get" },
	{ 0x9b15d336, "input_unregister_handle" },
	{ 0xe4de56b4, "__ubsan_handle_load_invalid_value" },
	{ 0xd9324140, "cdev_init" },
	{ 0x7851be11, "__SCT__might_resched" },
	{ 0xa65b83f9, "kmalloc_caches" },
	{ 0x73634a54, "cdev_del" },
	{ 0x1c098236, "input_allocate_device" },
	{ 0x9f222e1e, "alloc_chrdev_region" },
	{ 0x092a35a2, "_copy_from_user" },
	{ 0xb730487b, "finish_wait" },
	{ 0xd7442be0, "class_destroy" },
	{ 0xb524cc7d, "input_unregister_device" },
	{ 0xcb8b6ec6, "kfree" },
	{ 0xc281f1fb, "prepare_to_wait_event" },
	{ 0xe9196a28, "module_layout" },
};

static const u32 ____version_ext_crcs[]
__used __section("__version_ext_crcs") = {
	0x5e505530,
	0x68a1b6c6,
	0xc0481510,
	0x11f4259a,
	0xd272d446,
	0xa3a19834,
	0xb524cc7d,
	0xd272d446,
	0xe8213e80,
	0xbd03ed67,
	0x0e675b65,
	0xd272d446,
	0x69b2b2dd,
	0xd272d446,
	0x90a48d82,
	0x45d300cd,
	0x7a5ffe84,
	0x85be535a,
	0xc0481510,
	0xfad798b2,
	0xbd03ed67,
	0x2794f3c0,
	0x7057d579,
	0x9b15d336,
	0x444885a7,
	0x463753d3,
	0xd272d446,
	0x092a35a2,
	0xe804603d,
	0xfcf8942c,
	0x888b8f57,
	0x59cf8621,
	0xca31368d,
	0x35e43a03,
	0x0bc5fb0d,
	0x9aa6980d,
	0x0f889972,
	0x30eb81ed,
	0xce56ea17,
	0x97acb853,
	0x9b15d336,
	0xe4de56b4,
	0xd9324140,
	0x7851be11,
	0xa65b83f9,
	0x73634a54,
	0x1c098236,
	0x9f222e1e,
	0x092a35a2,
	0xb730487b,
	0xd7442be0,
	0xb524cc7d,
	0xcb8b6ec6,
	0xc281f1fb,
	0xe9196a28,
};
static const char ____version_ext_names[]
__used __section("__version_ext_names") =
	"kthread_should_stop\0"
	"__wake_up\0"
	"input_register_handle\0"
	"_raw_spin_lock_irqsave\0"
	"__fentry__\0"
	"wake_up_process\0"
	"input_free_device\0"
	"dump_stack\0"
	"_printk\0"
	"__ref_stack_chk_guard\0"
	"___ratelimit\0"
	"schedule\0"
	"input_register_device\0"
	"__stack_chk_fail\0"
	"__ubsan_handle_out_of_bounds\0"
	"cdev_add\0"
	"init_wait_entry\0"
	"device_create\0"
	"input_open_device\0"
	"class_create\0"
	"random_kmalloc_seed\0"
	"kthread_stop\0"
	"mutex_lock_interruptible\0"
	"input_close_device\0"
	"_raw_spin_unlock_irqrestore\0"
	"input_unregister_handler\0"
	"__x86_return_thunk\0"
	"_copy_to_user\0"
	"__init_waitqueue_head\0"
	"input_event\0"
	"strcmp\0"
	"nonseekable_open\0"
	"kthread_create_on_node\0"
	"input_register_handler\0"
	"unregister_chrdev_region\0"
	"mutex_unlock\0"
	"device_destroy\0"
	"__dynamic_pr_debug\0"
	"__kmalloc_cache_noprof\0"
	"ktime_get\0"
	"input_unregister_handle\0"
	"__ubsan_handle_load_invalid_value\0"
	"cdev_init\0"
	"__SCT__might_resched\0"
	"kmalloc_caches\0"
	"cdev_del\0"
	"input_allocate_device\0"
	"alloc_chrdev_region\0"
	"_copy_from_user\0"
	"finish_wait\0"
	"class_destroy\0"
	"input_unregister_device\0"
	"kfree\0"
	"prepare_to_wait_event\0"
	"module_layout\0"
;

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "92E4DAE01A0C391F3CDC6F4");
