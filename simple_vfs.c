#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>

#define FILE_SYSTEM_MAGIC 0x12345678

static const struct address_space_operations fs_aops;
static const struct inode_operations fs_inode_operations;
static const struct dentry_operations custom_fs_dentry_operations;

struct fs_data {
    struct super_block *sb;
    unsigned long long fib_num;
};

static const struct file_operations fs_file_operations = {
    .read = generic_read_dir
};

static const struct super_operations fs_super_operations = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

static const struct inode_operations fs_inode_operations = {
    .setattr = simple_setattr,
    .getattr = simple_getattr,
};

static unsigned long long fibonacci(unsigned long long n)
{
    if (n <= 1)
        return n;
    else
        return fibonacci(n - 1) + fibonacci(n - 2);
}

static int fs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    struct fs_data *fs_data;

    sb->s_magic = FILE_SYSTEM_MAGIC;
    sb->s_op = &fs_super_operations;

    root_inode = new_inode(sb);
    if (!root_inode)
        return -ENOMEM;

    root_inode->i_ino = 1;
    root_inode->i_sb = sb;
    root_inode->i_op = &fs_inode_operations;
    root_inode->i_fop = &fs_file_operations;
    root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root)
        return -ENOMEM;

    fs_data = kzalloc(sizeof(struct fs_data), GFP_KERNEL);
    if (!fs_data)
        return -ENOMEM;

    fs_data->sb = sb;
    fs_data->fib_num = 1;

    sb->s_fs_info = fs_data;

    return 0;
}

static struct inode *custom_fs_get_inode(struct super_block *sb, int mode)
{
    struct inode *inode;

    inode = new_inode(sb);
    if (!inode)
        return NULL;

    inode->i_mode = mode;
    inode->i_uid.val = 0;  // UID
    inode->i_gid.val = 0;  // GID
    inode->i_blocks = 0;   // num of blocks
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    inode->i_mapping->a_ops = &fs_aops;  // address space ops
    inode->i_op = &fs_inode_operations;  // inode ops

    return inode;
}

static ssize_t custom_fs_file_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct fs_data *fs_data = filp->f_path.dentry->d_sb->s_fs_info;
    char fib_num_str[64];
    ssize_t ret;

    // Calculate the next Fibonacci number
    fs_data->fib_num = fibonacci(fs_data->fib_num);

    // Convert the Fibonacci number to a string
    snprintf(fib_num_str, sizeof(fib_num_str), "%llu\n", fs_data->fib_num);

    // Copy the Fibonacci number string to the user buffer
    ret = simple_read_from_buffer(buf, len, ppos, fib_num_str, strlen(fib_num_str));

    return ret;
}

// Create the "/calc" directory
static int custom_fs_create_calc_directory(struct dentry *parent_dentry)
{
    struct dentry *calc_dentry;
    struct inode *calc_inode;
    struct qstr calc_name;

    calc_name.name = "calc";
    calc_name.len = strlen(calc_name.name);

    calc_dentry = d_alloc(parent_dentry, &calc_name);
    if (!calc_dentry) {
        pr_err("Failed to allocate dentry for /calc\n");
        return -ENOMEM;
    }

    calc_inode = custom_fs_get_inode(parent_dentry->d_sb, S_IFDIR | 0755);
    if (!calc_inode) {
        pr_err("Failed to allocate inode for /calc\n");
        dput(calc_dentry);
        return -ENOMEM;
    }

    calc_dentry->d_inode = calc_inode;
    calc_dentry->d_op = &custom_fs_dentry_operations;
    calc_dentry->d_sb = parent_dentry->d_sb;

    parent_dentry->d_sb->s_root = calc_dentry;

    return 0;
}

// Create the "hello.txt" file
static int custom_fs_create_hello_file(struct dentry *parent_dentry)
{
    struct dentry *hello_dentry;
    struct inode *hello_inode;
    struct qstr hello_name;

    hello_name.name = "hello.txt";
    hello_name.len = strlen(hello_name.name);

    hello_dentry = d_alloc(parent_dentry, &hello_name);
    if (!hello_dentry) {
        pr_err("Failed to allocate dentry for hello.txt\n");
        return -ENOMEM;
    }

    hello_inode = custom_fs_get_inode(parent_dentry->d_sb, S_IFREG | 0644);
    if (!hello_inode) {
        pr_err("Failed to allocate inode for hello.txt\n");
        dput(hello_dentry);
        return -ENOMEM;
    }

    hello_dentry->d_inode = hello_inode;
    hello_dentry->d_op = &custom_fs_dentry_operations;
    hello_dentry->d_sb = parent_dentry->d_sb;

    hello_inode->i_fop = &fs_file_operations;

    inc_nlink(hello_inode);
    d_instantiate(hello_dentry, hello_inode);

    return 0;
}

// Create the "/calc/fib.num" file
static int custom_fs_create_fib_num_file(struct dentry *parent_dentry)
{
    struct dentry *fib_num_dentry;
    struct inode *fib_num_inode;
    struct qstr fib_num_name;

    fib_num_name.name = "fib.num";
    fib_num_name.len = strlen(fib_num_name.name);

    fib_num_dentry = d_alloc(parent_dentry, &fib_num_name);
    if (!fib_num_dentry) {
        pr_err("Failed to allocate dentry for /calc/fib.num\n");
        return -ENOMEM;
    }

    fib_num_inode = custom_fs_get_inode(parent_dentry->d_sb, S_IFREG | 0644);
    if (!fib_num_inode) {
        pr_err("Failed to allocate inode for /calc/fib.num\n");
        dput(fib_num_dentry);
        return -ENOMEM;
    }

    fib_num_dentry->d_inode = fib_num_inode;
    fib_num_dentry->d_op = &custom_fs_dentry_operations;
    fib_num_dentry->d_sb = parent_dentry->d_sb;

    fib_num_inode->i_fop = &fs_file_operations;

    inc_nlink(fib_num_inode);
    d_instantiate(fib_num_dentry, fib_num_inode);

    return 0;
}

// Mount the custom file system
static struct dentry *custom_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    struct dentry *entry;
    struct fs_data *fs_data;

    // Attempt to mount the file system
    entry = mount_nodev(fs_type, flags, data, fs_fill_super);
    if (IS_ERR(entry)) {
        pr_err("Failed to mount the custom file system\n");
        return entry;
    }

    // Allocate memory for custom file system data
    fs_data = kzalloc(sizeof(struct fs_data), GFP_KERNEL);
    if (!fs_data) {
        pr_err("Failed to allocate memory for fs_data\n");
        goto out_fail;
    }

    fs_data->sb = entry->d_sb->s_fs_info;
    entry->d_sb->s_fs_info = fs_data;

    // Create the "/calc" directory
    if (custom_fs_create_calc_directory(entry) != 0)
        goto out_fail;

    // Create the "hello.txt" file
    if (custom_fs_create_hello_file(entry->d_sb->s_root) != 0)
        goto out_fail;

    // Create the "/calc/fib.num" file
    if (custom_fs_create_fib_num_file(entry->d_sb->s_root) != 0)
        goto out_fail;

    return entry;

out_fail:
    // Cleanup on failure
    deactivate_locked_super(entry->d_sb);
    return ERR_PTR(-ENOMEM);
}

static void custom_fs_kill_super(struct super_block *sb)
{
    struct fs_data *fs_data = sb->s_fs_info;

    kill_block_super(sb);

    pr_info("Unmounted disk\n");

    kfree(fs_data);
}

// Custom file system file system type
static struct file_system_type custom_fs_type = {
    .name = "customfs",
    .mount = custom_fs_mount,
    .kill_sb = custom_fs_kill_super,
};

// Initialize the custom file system module
static int __init custom_fs_init(void)
{
    int ret;

    ret = register_filesystem(&custom_fs_type);
    if (ret != 0) {
        pr_err("Failed to register custom file system\n");
        return ret;
    }

    pr_info("Custom file system module loaded\n");

    return 0;
}

// Clean up the custom file system module
static void __exit custom_fs_exit(void)
{
    unregister_filesystem(&custom_fs_type);

    pr_info("Custom file system module unloaded\n");
}

module_init(custom_fs_init);
module_exit(custom_fs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haim Kasel");
MODULE_DESCRIPTION("Simple Virtual File System Module, Creates Read Only /hello.txt and /calc/fib.num Files.");