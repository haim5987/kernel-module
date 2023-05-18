#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/uaccess.h>

/* Your code goes here */

MODULE_LICENSE("GPL");

/* Initialization and cleanup functions */
static int __init mysyscall_init(void)
{
    /* Initialization code */
    printk("New sys call");
    return 0;
}

static void __exit mysyscall_exit(void)
{
    /* Cleanup code */
}

module_init(mysyscall_init);
module_exit(mysyscall_exit);
