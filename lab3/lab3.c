#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("felix");
MODULE_DESCRIPTION("TSU Linux Module");

static int __init tsu_module_init(void)
{
    printk(KERN_INFO "Welcome to the Tomsk State University\n");
    return 0;
}

static void __exit tsu_module_exit(void)
{
    printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(tsu_module_init);
module_exit(tsu_module_exit);
