#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define EXAMPLEFS_MAGIC_NUMBER 0x73617361 // Magic number for the file system

static struct inode *examplefs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode, dev_t dev)
{
    struct inode *inode = new_inode(sb);
    if (inode) {
        inode->i_ino = get_next_ino();
        inode_init_owner(inode, dir, mode);
        inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
        switch (mode & S_IFMT) {
            case S_IFDIR:
                inode->i_op = &simple_dir_inode_operations;
                inode->i_fop = &simple_dir_operations;
                break;
            case S_IFREG:
                inode->i_op = &simple_file_inode_operations;
                inode->i_fop = &simple_file_operations;
                break;
            default:
                printk(KERN_ERR "Unsupported file type.\n");
                iput(inode);
                return NULL;
        }
    }
    return inode;
}

static int examplefs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    struct dentry *root_dentry;

    sb->s_magic = EXAMPLEFS_MAGIC_NUMBER;
    sb->s_blocksize = PAGE_CACHE_SIZE;
    sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
    sb->s_op = &simple_super_operations;

    root_inode = examplefs_get_inode(sb, NULL, S_IFDIR | 0755, 0);
    if (!root_inode) {
        printk(KERN_ERR "Failed to create root inode.\n");
        return -ENOMEM;
    }

    root_dentry = d_make_root(root_inode);
    if (!root_dentry) {
        printk(KERN_ERR "Failed to create root dentry.\n");
        iput(root_inode);
        return -ENOMEM;
    }

    sb->s_root = root_dentry;
    return 0;
}

static struct dentry *examplefs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    struct dentry *ret;
    ret = mount_nodev(fs_type, flags, data, examplefs_fill_super);
    if (unlikely(IS_ERR(ret)))
        printk(KERN_ERR "Failed to mount examplefs.\n");
    else
        printk(KERN_INFO "examplefs mounted.\n");
    return ret;
}

static void examplefs_kill_superblock(struct super_block *sb)
{
    printk(KERN_INFO "Unmounting examplefs.\n");
    kill_litter_super(sb);
}

static struct file_system_type examplefs_type = {
    .owner = THIS_MODULE,
    .name = "examplefs",
    .mount = examplefs_mount,
    .kill_sb = examplefs_kill_superblock,
};

static int __init examplefs_init(void)
{
    int ret;

    ret = register_filesystem(&examplefs_type);
    if (likely(ret == 0))
        printk(KERN_INFO "Successfully registered examplefs.\n");
    else
        printk(KERN_ERR "Failed to register examplefs. Error: %d\n", ret);

    return ret;
}

static void __exit examplefs_exit(void)
{
    int ret;

    ret = unregister_filesystem(&examplefs_type);
    if (likely(ret == 0))
        printk(KERN_INFO "Successfully unregistered examplefs.\n");
    else
        printk(KERN_ERR "Failed to unregister examplefs. Error: %d\n", ret);
}

module_init(examplefs_init);
module_exit(examplefs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Example read-only file system");