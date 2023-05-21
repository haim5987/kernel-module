#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/slab.h>

#define FILE_SYSTEM_MAGIC 0x12345678

static const struct address_space_operations custom_fs_aops;
static const struct inode_operations custom_fs_inode_operations;
static const struct dentry_operations custom_fs_dentry_operations;

// Structure to hold custom file system data
struct custom_fs_data {
    struct super_block *sb;
    unsigned long long fib_num;  // Next number of Fibonacci sequence
};

// Custom file system file operations
static const struct file_operations custom_fs_file_operations = {
    .read = generic_read_dir
};

// Custom file system superblock operations
static const struct super_operations custom_fs_super_operations = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

// Custom file system inode operations
static const struct inode_operations custom_fs_inode_operations = {
    .setattr = simple_setattr,
    .getattr = simple_getattr,
};

// Fibonacci sequence calculation
static unsigned long long fibonacci(unsigned long long n)
{
    if (n <= 1)
        return n;
    else
        return fibonacci(n - 1) + fibonacci(n - 2);
}

// Initialize custom file system superblock
static int custom_fs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode;
    struct custom_fs_data *fs_data;

    sb->s_magic = FILE_SYSTEM_MAGIC;
    sb->s_op = &custom_fs_super_operations;

    root_inode = new_inode(sb);
    if (!root_inode)
        return -ENOMEM;

    root_inode->i_ino = 1;
    root_inode->i_sb = sb;
    root_inode->i_op = &custom_fs_inode_operations;
    root_inode->i_fop = &custom_fs_file_operations;
    root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root)
        return -ENOMEM;

    // Allocate memory for custom file system data
    fs_data = kzalloc(sizeof(struct custom_fs_data), GFP_KERNEL);
    if (!fs_data)
        return -ENOMEM;

    fs_data->sb = sb;
    fs_data->fib_num = 1;  // Initialize with the first number of the Fibonacci sequence

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
    inode->i_uid.val = 0;  // Set the UID (user ID) of the inode
    inode->i_gid.val = 0;  // Set the GID (group ID) of the inode
    inode->i_blocks = 0;   // Set the number of blocks used by the inode
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);  // Set the access, modification, and change times
    inode->i_mapping->a_ops = &custom_fs_aops;  // Set the address space operations
    inode->i_op = &custom_fs_inode_operations;  // Set the inode operations

    // Additional initialization specific to your custom file system

    return inode;
}

// Custom file system file read operation
static ssize_t custom_fs_file_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct custom_fs_data *fs_data = filp->f_path.dentry->d_sb->s_fs_info;
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

// Mount the custom file system
static struct dentry *custom_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    struct dentry *entry;
    struct custom_fs_data *fs_data;
    struct dentry *file_dentry;
    struct inode *file_inode;
    struct qstr file_name;

    // Attempt to mount the file system
    entry = mount_nodev(fs_type, flags, data, custom_fs_fill_super);
    if (IS_ERR(entry)) {
        pr_err("Failed to mount the custom file system\n");
        return entry;
    }

    // Allocate memory for custom file system data
    fs_data = kzalloc(sizeof(struct custom_fs_data), GFP_KERNEL);
    if (!fs_data) {
        pr_err("Failed to allocate memory for custom_fs_data\n");
        goto out_fail;
    }

    fs_data->sb = entry->d_sb->s_fs_info;
    entry->d_sb->s_fs_info = fs_data;

    // Create the "/calc" directory
    file_name.name = "calc";
    file_name.len = strlen(file_name.name);

    file_dentry = d_alloc(entry, &file_name);
    if (!file_dentry) {
        pr_err("Failed to allocate dentry for /calc\n");
        goto out_fail;
    }

    file_inode = custom_fs_get_inode(entry->d_sb, S_IFDIR | 0755);
    if (!file_inode) {
        pr_err("Failed to allocate inode for /calc\n");
        goto out_fail;
    }

    file_dentry->d_inode = file_inode;
    file_dentry->d_op = &custom_fs_dentry_operations;
    file_dentry->d_sb = entry->d_sb;

    entry->d_sb->s_root = file_dentry;

    // Create the "hello.txt" file
    file_name.name = "hello.txt";
    file_name.len = strlen(file_name.name);

    file_dentry = d_alloc(entry, &file_name);
    if (!file_dentry) {
        pr_err("Failed to allocate dentry for hello.txt\n");
        goto out_fail;
    }

    file_inode = custom_fs_get_inode(entry->d_sb, S_IFREG | 0644);
    if (!file_inode) {
        pr_err("Failed to allocate inode for hello.txt\n");
        goto out_fail;
    }

    file_dentry->d_inode = file_inode;
    file_dentry->d_op = &custom_fs_dentry_operations;
    file_dentry->d_sb = entry->d_sb;

    file_inode->i_fop = &custom_fs_file_operations;

    inc_nlink(file_inode);
    d_instantiate(file_dentry, file_inode);

    // Create the "/calc/fib.num" file
    file_name.name = "fib.num";
    file_name.len = strlen(file_name.name);

    file_dentry = d_alloc(file_dentry, &file_name);
    if (!file_dentry) {
        pr_err("Failed to allocate dentry for /calc/fib.num\n");
        goto out_fail;
    }

    file_inode = custom_fs_get_inode(entry->d_sb, S_IFREG | 0644);
    if (!file_inode) {
        pr_err("Failed to allocate inode for /calc/fib.num\n");
        goto out_fail;
    }

    file_dentry->d_inode = file_inode;
    file_dentry->d_op = &custom_fs_dentry_operations;
    file_dentry->d_sb = entry->d_sb;

    file_inode->i_fop = &custom_fs_file_operations;

    inc_nlink(file_inode);
    d_instantiate(file_dentry, file_inode);

    return entry;

out_fail:
    // Cleanup on failure
    deactivate_locked_super(entry->d_sb);
    return ERR_PTR(-ENOMEM);
}

static void custom_fs_kill_super(struct super_block *sb)
{
    struct custom_fs_data *fs_data = sb->s_fs_info;

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
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Custom File System Module");