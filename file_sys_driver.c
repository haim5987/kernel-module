#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haim Kasel");
MODULE_DESCRIPTION("Read-only Hello World file system driver");


// Your file system superblock operations
static struct super_operations myfs_sb_ops = {
    // Fill in the necessary superblock operations
    // For example, .statfs = your_statfs_function,
    //               .drop_inode = your_drop_inode_function,
};

static struct dentry *hello_mount_callback(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    printk(KERN_INFO "Mount successful!\n");
    
    struct dentry *ret;

    // Create the root dentry for your file system
    ret = mount_bdev(fs_type, flags, dev_name, data, simple_fill_super);
    if (IS_ERR(ret))
        printk(KERN_ERR "Failed to mount the file system.\n");
    else
        printk(KERN_INFO "File system mounted successfully.\n");

    return ret;
}


static struct file_system_type read_only_fs_type = {
    .name = "hello",
    .kill_sb = kill_litter_super,
    .mount = hello_mount_callback,
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
