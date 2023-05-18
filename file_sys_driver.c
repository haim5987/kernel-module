#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haim Kasel");
MODULE_DESCRIPTION("Read-only Hello World file system driver");


static struct file_system_type read_only_fs_type = {
    .name = "hello",
    .kill_sb = kill_litter_super,
};

static int __init read_only_fs_init(void) {
    int ret = register_filesystem(&read_only_fs_type);
    if (ret != 0) {
        pr_err("Failed to register read only file system\n");
        return ret;
    }

    pr_info("read only file system module loaded\n");
    return 0;
}


static void __exit read_only_fs_exit(void) {
    int ret = unregister_filesystem(&read_only_fs_type);
    if (ret != 0)
        pr_err("Failed to unregister read only file system\n");

    pr_info("read only file system module unloaded\n");
}

module_init(read_only_fs_init);
module_exit(read_only_fs_exit);
