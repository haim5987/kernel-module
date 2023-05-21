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


struct inode *myfs_get_inode(struct super_block *sb, int mode)
{
    struct inode *inode = new_inode(sb);
    if (!inode)
        return NULL;

    inode->i_mode = mode;
    inode->i_uid.val = current_uid().val;
    inode->i_gid.val = current_gid().val;
    inode->i_blocks = 0;
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    switch (mode & S_IFMT) {
        case S_IFREG:
            inode->i_op = simple_dir_inode_operations;
            inode->i_fop = file_operations;
            break;
        case S_IFDIR:
            inode->i_op = simple_dir_inode_operations;
            inode->i_fop = simple_dir_operations;
            inc_nlink(inode);
            break;
        default:
            init_special_inode(inode, mode, 0);
            break;
    }

    return inode;
}

static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
    // Perform any necessary initialization for your file system's superblock
    // For example:
    
    // Set the file system magic number
    sb->s_magic = 0x4D59;

    // Set the file system operations
    sb->s_op = &myfs_sb_ops;

    // Set up the root inode
    struct inode *root_inode = NULL;
    root_inode = myfs_get_inode(sb, S_IFDIR | S_IRWXU);
    if (!root_inode)
    {
        printk(KERN_ERR "Failed to allocate root inode.\n");
        return -ENOMEM;
    }

    // Set the root inode as the root of the file system
    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root)
    {
        printk(KERN_ERR "Failed to create root dentry.\n");
        iput(root_inode);
        return -ENOMEM;
    }

    return 0;
}

static struct dentry *hello_mount_callback(struct file_system_type *fs_type, int flags, const char *dev_name, void *data) {
    printk(KERN_INFO "Mount successful!\n");
    
    struct dentry *ret;

    // Create the root dentry for your file system
    ret = mount_bdev(fs_type, flags, dev_name, data, myfs_fill_super);
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
