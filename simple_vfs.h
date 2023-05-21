#include <linux/fs.h>

#define FILE_SYSTEM_MAGIC 0x12345678

// extern const struct file_operations custom_fs_file_operations;
// extern const struct inode_operations custom_fs_inode_operations;
// extern const struct dentry_operations custom_fs_dentry_operations;

// struct custom_fs_data {
//     struct super_block *sb;
//     unsigned long long fib_num;
// };

extern const struct super_operations custom_fs_super_operations;

struct inode *custom_fs_get_inode(struct super_block *sb, int mode);

ssize_t custom_fs_file_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos);

struct dentry *custom_fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data);

void custom_fs_kill_super(struct super_block *sb);