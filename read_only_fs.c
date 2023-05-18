#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/pagemap.h>

#define RAMFS_MAGIC     0x858458f6

static const struct super_operations ramfs_ops = {
        .statfs         = simple_statfs,
        .drop_inode     = generic_delete_inode,
        .show_options   = ramfs_show_options,
};

static int ramfs_fill_super(struct super_block *sb, void *data, int silent)
{
        struct ramfs_fs_info *fsi;
        struct inode *inode;
        int err;

        save_mount_options(sb, data);

        fsi = kzalloc(sizeof(struct ramfs_fs_info), GFP_KERNEL);
        sb->s_fs_info = fsi;
        if (!fsi)
                return -ENOMEM;

        err = ramfs_parse_options(data, &fsi->mount_opts);
        if (err)
                return err;

        sb->s_maxbytes          = MAX_LFS_FILESIZE;
        sb->s_blocksize         = PAGE_SIZE;
        sb->s_blocksize_bits    = PAGE_SHIFT;
        sb->s_magic             = RAMFS_MAGIC;
        sb->s_op                = &ramfs_ops;
        sb->s_time_gran         = 1;

        inode = ramfs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
        sb->s_root = d_make_root(inode);
        if (!sb->s_root)
                return -ENOMEM;

        return 0;
}


struct dentry *ramfs_mount(struct file_system_type *fs_type,
        int flags, const char *dev_name, void *data)
{
        return mount_nodev(fs_type, flags, data, ramfs_fill_super);
}



static struct file_system_type ramfs_fs_type = {
        .name           = "ramfs",
        .mount          = ramfs_mount,
        .kill_sb        = ramfs_kill_sb,
        .fs_flags       = FS_USERNS_MOUNT,
};





static int __init init_ramfs_fs(void)
{
        if (test_and_set_bit(0, &once))
                return 0;
        return register_filesystem(&ramfs_fs_type);
}