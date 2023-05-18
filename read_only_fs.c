#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/slab.h>

#define FILE_CONTENT_HELLO "Hello world!\n"
#define FILE_CONTENT_FIB "1\n"

static struct file_system_type myfs_type = {
    .owner = THIS_MODULE,
    .name = "myfs",
};

static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    struct dentry *root_dentry;

    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = 0x12345678; // Unique identifier for the file system

    root_inode = new_inode(sb);
    if (!root_inode)
        return -ENOMEM;

    root_inode->i_ino = 1;
    root_inode->i_sb = sb;
    root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = CURRENT_TIME;
    root_inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO;
    root_inode->i_op = &simple_dir_inode_operations;
    root_inode->i_fop = &simple_dir_operations;

    root_dentry = d_make_root(root_inode);
    if (!root_dentry) {
        iput(root_inode);
        return -ENOMEM;
    }

    sb->s_root = root_dentry;

    // Create hello.txt
    struct dentry *hello_dentry = d_alloc_name(root_dentry, "hello.txt");
    if (!hello_dentry)
        return -ENOMEM;

    struct inode *hello_inode = new_inode(sb);
    if (!hello_inode) {
        dput(hello_dentry);
        return -ENOMEM;
    }

    hello_inode->i_ino = 2;
    hello_inode->i_sb = sb;
    hello_inode->i_atime = hello_inode->i_mtime = hello_inode->i_ctime = CURRENT_TIME;
    hello_inode->i_mode = S_IFREG | S_IRUGO;
    hello_inode->i_op = &simple_file_inode_operations;
    hello_inode->i_fop = &simple_file_operations;

    struct file *hello_file = file_open_root(hello_dentry->d_inode, hello_dentry, FMODE_READ, NULL);
    if (IS_ERR(hello_file)) {
        iput(hello_inode);
        dput(hello_dentry);
        return PTR_ERR(hello_file);
    }

    hello_file->f_path.dentry->d_inode = hello_inode;
    hello_file->f_path.dentry->d_parent = root_dentry;

    // Set file contents for hello.txt
    loff_t hello_size = strlen(FILE_CONTENT_HELLO);
    hello_inode->i_size = hello_size;
    hello_file->f_path.dentry->d_inode->i_size = hello_size;
    memcpy(hello_file->f_path.dentry->d_inode->i_private, FILE_CONTENT_HELLO, hello_size);

    // Create calc directory
    struct dentry *calc_dentry = d_alloc_name(root_dentry, "calc");
    if (!calc_dentry) {
        iput(hello_inode);
        dput(hello_dentry);
        return -ENOMEM;
    }

    struct inode *calc_inode = new_inode(sb);
    if (!calc_inode) {
        iput(hello_inode);
        dput(hello_dentry);
        dput(calc_dentry);
        return -ENOMEM;
    }

    calc_inode->i_ino = 3;
    calc_inode->i_sb = sb;
    calc_inode->i_atime = calc_inode->i_mtime = calc_inode->i_ctime = CURRENT_TIME;
    calc_inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO;
    calc_inode->i_op = &simple_dir_inode_operations;
    calc_inode->i_fop = &simple_dir_operations;

    calc_dentry->d_inode = calc_inode;
    calc_dentry->d_parent = root_dentry;

    // Create fib.num
    struct dentry *fib_dentry = d_alloc_name(calc_dentry, "fib.num");
    if (!fib_dentry) {
        iput(hello_inode);
        dput(hello_dentry);
        iput(calc_inode);
        dput(calc_dentry);
        return -ENOMEM;
    }

    struct inode *fib_inode = new_inode(sb);
    if (!fib_inode) {
        iput(hello_inode);
        dput(hello_dentry);
        iput(calc_inode);
        dput(calc_dentry);
        dput(fib_dentry);
        return -ENOMEM;
    }

    fib_inode->i_ino = 4;
    fib_inode->i_sb = sb;
    fib_inode->i_atime = fib_inode->i_mtime = fib_inode->i_ctime = CURRENT_TIME;
    fib_inode->i_mode = S_IFREG | S_IRUGO;
    fib_inode->i_op = &simple_file_inode_operations;
    fib_inode->i_fop = &simple_file_operations;

    struct file *fib_file = file_open_root(fib_dentry->d_inode, fib_dentry, FMODE_READ, NULL);
    if (IS_ERR(fib_file)) {
        iput(hello_inode);
        dput(hello_dentry);
        iput(calc_inode);
        dput(calc_dentry);
        iput(fib_inode);
        dput(fib_dentry);
        return PTR_ERR(fib_file);
    }

    fib_file->f_path.dentry->d_inode = fib_inode;
    fib_file->f_path.dentry->d_parent = calc_dentry;

    // Set file contents for fib.num
    loff_t fib_size = strlen(FILE_CONTENT_FIB);
    fib_inode->i_size = fib_size;
    fib_file->f_path.dentry->d_inode->i_size = fib_size;
    memcpy(fib_file->f_path.dentry->d_inode->i_private, FILE_CONTENT_FIB, fib_size);

    return 0;
}

static struct dentry *myfs_mount(struct file_system_type *fs_type, int flags,
                                 const char *dev_name, void *data)
{
    struct dentry *dentry;
    struct super_block *s;

    s = sget(fs_type, NULL, set_anon_super, flags, NULL);
    if (IS_ERR(s))
        return ERR_CAST(s);

    s->s_flags |= MS_RDONLY;
    s->s_op = &simple_super_operations;

    dentry = mount_nodev(fs_type, flags, NULL, myfs_fill_super);
    if (IS_ERR(dentry)) {
        kill_anon_super(s);
        return ERR_CAST(dentry);
    }

    return dentry;
}

static void myfs_kill_superblock(struct super_block *sb)
{
    kill_anon_super(sb);
}

static struct file_system_type myfs_type = {
    .owner = THIS_MODULE,
    .name = "myfs",
    .mount = myfs_mount,
    .kill_sb = myfs_kill_superblock,
};

static int __init myfs_init(void)
{
    return register_filesystem(&myfs_type);
}

static void __exit myfs_exit(void)
{
    unregister_filesystem(&myfs_type);
}

module_init(myfs_init);
module_exit(myfs_exit);
MODULE_LICENSE("GPL");