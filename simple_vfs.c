#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fs.h>

#define FILE_SYSTEM_MAGIC 0x12345678
#define BLOCK_SIZE (1 << 12) /* 4 KiB */
#define MAX_EXTENTS \
    ((BLOCK_SIZE - sizeof(uint32_t)) / sizeof(struct fs_extent))
#define MAX_BLOCKS_PER_EXTENT 8 /* It can be ~(uint32) 0 */
#define MAX_FILESIZE                                      \
    ((uint64_t) MAX_BLOCKS_PER_EXTENT *BLOCK_SIZE \
        * MAX_EXTENTS)

static const struct address_space_operations fs_aops;
static const struct inode_operations fs_inode_operations;
static const struct dentry_operations custom_fs_dentry_operations;
struct super_block *sb;

struct fs_extent {
    uint32_t ee_block; /* first logical block extent covers */
    uint32_t ee_len;   /* number of blocks covered by extent */
    uint32_t ee_start; /* first physical block extent covers */
};

struct fs_data {
    struct super_block *sb;
    unsigned long long fib_num;
};

static const struct file_operations fs_file_operations = {
    .read = generic_read_dir
};

static const struct super_operations fs_super_operations = {
    .alloc_inode     = alloc_inode,
    .destroy_inode   = destroy_inode,
    .dirty_inode     = dirty_inode,
    .write_inode     = write_inode,
    .drop_inode      = drop_inode,
    .delete_inode    = delete_inode,
    .put_super       = put_super,
    .write_super     = write_super,
    .sync_fs         = sync_fs,
    .freeze_fs       = freeze_super,
    .unfreeze_fs     = thaw_super,
    .statfs          = default_statfs,
    .remount_fs      = remount_fs,
    .show_options    = generic_show_options,
    .show_devname    = simple_show_devname,
    .show_path       = simple_show_path,
    .show_stats      = simple_show_stats,
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
    printk("fs_fill_super start");
    struct inode *root_inode;
    struct fs_data *fs_data;

    printk("sb start");
    sb->s_magic = FILE_SYSTEM_MAGIC;
    printk("sb->s_magic finish");
    sb_set_blocksize(sb, BLOCK_SIZE);
    printk("sb_set_blocksize finish");
    sb->s_maxbytes = MAX_FILESIZE;
    printk("sb->s_maxbytes finish");
    sb->s_op = &fs_super_operations;

    printk("sb finish");
    printk("root_inode start");
    root_inode = new_inode(sb);
    if (!root_inode)
        return -ENOMEM;

    root_inode->i_ino = 1;
    root_inode->i_sb = sb;
    root_inode->i_op = &fs_inode_operations;
    root_inode->i_fop = &fs_file_operations;
    root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);

    printk("root_inode finish");
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

static struct inode *fs_get_inode(struct super_block *sb, int mode)
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

static ssize_t fs_file_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct fs_data *fs_data = filp->f_path.dentry->d_sb->s_fs_info;
    char fib_num_str[64];
    ssize_t ret;

    fs_data->fib_num = fibonacci(fs_data->fib_num);  // next Fibonacci num
    snprintf(fib_num_str, sizeof(fib_num_str), "%llu\n", fs_data->fib_num);  // fib num to str
    ret = simple_read_from_buffer(buf, len, ppos, fib_num_str, strlen(fib_num_str));   // fib num str to user buffer

    return ret;
}

static int fs_create_calc_directory(struct dentry *parent_dentry)
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

    calc_inode = fs_get_inode(parent_dentry->d_sb, S_IFDIR | 0755);
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

static int fs_create_hello_file(struct dentry *parent_dentry)
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

    hello_inode = fs_get_inode(parent_dentry->d_sb, S_IFREG | 0644);
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

static int fs_create_fib_num_file(struct dentry *parent_dentry)
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

    fib_num_inode = fs_get_inode(parent_dentry->d_sb, S_IFREG | 0644);
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

static struct dentry *fs_mount(struct file_system_type *fs_type, int flags, const char *dev_name, void *data)
{
    struct dentry *entry;
    struct fs_data *fs_data;

    printk("mount_nodev start");

    entry = mount_nodev(fs_type, flags, data, fs_fill_super);
    if (IS_ERR(entry)) {
        pr_err("Failed to mount the custom file system\n");
        return entry;
    }

    printk("mount_nodev finish");

    printk("kzalloc start");

    fs_data = kzalloc(sizeof(struct fs_data), GFP_KERNEL);
    if (!fs_data) {
        pr_err("Failed to allocate memory for fs_data\n");
        goto out_fail;
    }

    fs_data->sb = entry->d_sb->s_fs_info;
    entry->d_sb->s_fs_info = fs_data;

    printk("kzalloc finsih");
    printk("fs_create_calc_directory start");
    if (fs_create_calc_directory(entry) != 0) // create /calc dir
        goto out_fail;
     printk("fs_create_calc_directory finsih");
    
    printk("fs_create_hello_file start");
    if (fs_create_hello_file(entry->d_sb->s_root) != 0) // create hello.txt file
        goto out_fail;

    printk("fs_create_hello_file finsih");
    if (fs_create_fib_num_file(entry->d_sb->s_root) != 0) // create /calc/fib.num file
        goto out_fail;

    return entry;

out_fail:
    deactivate_locked_super(entry->d_sb);
    return ERR_PTR(-ENOMEM);
}

static void fs_kill_super(struct super_block *sb)
{
    printk("fs_data *fs_data start");
    struct fs_data *fs_data = sb->s_fs_info;

    printk("fs_data *fs_data finish");

     printk("kill_block_super start");
    // if (sb){
    //     kill_block_super(sb);
    // }
    

    printk("kill_block_super finish");
    pr_info("Unmounted disk\n");

    kfree(fs_data);
}

static struct file_system_type fs_type = {
    .name = "vfs",
    .mount = fs_mount,
    .kill_sb = fs_kill_super,
    .fs_flags = FS_REQUIRES_DEV,
    .next = NULL,
};

static int __init fs_init(void)
{
    int ret;

    ret = register_filesystem(&fs_type);
    if (ret != 0) {
        pr_err("Failed to register custom file system\n");
        return ret;
    }

    pr_info("Custom file system module loaded\n");

    return 0;
}

static void __exit fs_exit(void)
{
    unregister_filesystem(&fs_type);

    pr_info("Custom file system module unloaded\n");
}

module_init(fs_init);
module_exit(fs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haim Kasel");
MODULE_DESCRIPTION("Simple Virtual File System Module, Creates Read Only /hello.txt and /calc/fib.num Files.");