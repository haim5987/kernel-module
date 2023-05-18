#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haim Kasel");
MODULE_DESCRIPTION("Read-only Hello World file system driver");


static struct dentry *hello_mount_callback(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    printk(KERN_INFO "Mount successful!\n");
    
    struct dentry *root_dentry = NULL;
    struct inode *root_inode = NULL;
    struct dentry *file_dentry = NULL;
    struct file *file = NULL;
    struct super_block *sb;

    int error = 0;

    // Get the superblock from the file_system_type
    sb = get_sb_nodev(fs_type, flags, dev_name, data);
    if (IS_ERR(sb))
    {
        error = PTR_ERR(sb);
        return error;
    }

    // Create the root inode
    root_inode = new_inode(sb);
    if (!root_inode)
    {
        error = -ENOMEM;
        goto out_sb;
    }


out_file:
    if (file)
        fput(file);
out_dentry:
    if (file_dentry)
        dput(file_dentry);
out_inode:
    if (root_dentry)
        dput(root_dentry);
out_sb:
    if (sb)
        deactivate_locked_super(sb);

    return error;
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
