#include <linux/fs.h>

#define FILE_SYSTEM_MAGIC 0x12345678

static const struct dentry_operations custom_fs_dentry_operations;

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

static const struct file_operations custom_fs_file_operations = {
    .read = generic_read_dir
};

struct custom_fs_data {
    struct super_block *sb;
    unsigned long long fib_num;
};

extern const struct super_operations custom_fs_super_operations;

struct inode *custom_fs_get_inode(struct super_block *sb, int mode);

ssize_t custom_fs_file_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos);

struct dentry *custom_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);

void custom_fs_kill_super(struct super_block *sb);