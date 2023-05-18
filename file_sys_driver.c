#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haim Kasel");
MODULE_DESCRIPTION("Read-only Hello World file system driver");


#define HELLO_FILE_CONTENT "Hello world!\n"
#define HELLO_FILE_SIZE (sizeof(HELLO_FILE_CONTENT) - 1)

static struct file_operations hello_fops;

static struct inode *hello_inode(struct super_block *sb, int mode)
{
    struct inode *inode = new_inode(sb);
    if (inode) {
        inode->i_ino = get_next_ino();
        inode->i_mode = mode;
        inode->i_op = &hello_fops;
        inode->i_fop = &hello_fops;
        inode->i_size = HELLO_FILE_SIZE;
    }
    return inode;
}

static ssize_t hello_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    return simple_read_from_buffer(buf, len, off, HELLO_FILE_CONTENT, HELLO_FILE_SIZE);
}

static int __init hello_init(void)
{
    struct dentry *dentry;
    struct super_block *sb;
    struct inode *inode;

    sb = sget_userns(NULL, &hello_fops, NULL, 0, MS_RDONLY);
    if (IS_ERR(sb)) {
        pr_err("Failed to create superblock\n");
        return PTR_ERR(sb);
    }

    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;

    dentry = d_make_root(inode);
    if (!dentry) {
        pr_err("Failed to create root dentry\n");
        return -ENOMEM;
    }

    sb->s_root = dentry;
    inode = hello_inode(sb, S_IFREG | S_IRUGO);
    if (!inode) {
        pr_err("Failed to create inode\n");
        return -ENOMEM;
    }
    d_add(dentry, inode);

    return 0;
}

static void __exit hello_exit(void)
{
    return;
}

static struct file_operations hello_fops = {
    .read = hello_read,
};

module_init(hello_init);
module_exit(hello_exit);
