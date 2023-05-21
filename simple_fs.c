#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>



static struct file_system_type simplefs_file_system_type = {
    .owner = THIS_MODULE,
    .name = "simplefs",
    .mount = simplefs_mount,
    // .kill_sb = simplefs_kill_sb,
    .fs_flags = FS_REQUIRES_DEV,
    .next = NULL,
};

static int __init simplefs_init(void)
{
    ret = register_filesystem(&simplefs_file_system_type);
    if (ret) {
        pr_err("register_filesystem() failed\n");
        goto end;
    }

    pr_info("module loaded\n");
end:
    return ret;
}

static void __exit simplefs_exit(void)
{
    int ret = unregister_filesystem(&simplefs_file_system_type);
    if (ret)
        pr_err("unregister_filesystem() failed\n");

    pr_info("module unloaded\n");
}



module_init(simplefs_init);
module_exit(simplefs_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("a simple file system");