#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/statfs.h>


int simplefs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct buffer_head *bh = NULL;
    struct simplefs_sb_info *csb = NULL;
    struct simplefs_sb_info *sbi = NULL;
    struct inode *root_inode = NULL;
    int ret = 0, i;

//     /* Init sb */
//     sb->s_magic = SIMPLEFS_MAGIC;
//     sb_set_blocksize(sb, SIMPLEFS_BLOCK_SIZE);
//     sb->s_maxbytes = SIMPLEFS_MAX_FILESIZE;
//     sb->s_op = &simplefs_super_ops;

//     /* Read sb from disk */
//     bh = sb_bread(sb, SIMPLEFS_SB_BLOCK_NR);
//     if (!bh)
//         return -EIO;

//     csb = (struct simplefs_sb_info *) bh->b_data;

//     /* Check magic number */
//     if (csb->magic != sb->s_magic) {
//         pr_err("Wrong magic number\n");
//         ret = -EINVAL;
//         goto release;
//     }

//     /* Alloc sb_info */
//     sbi = kzalloc(sizeof(struct simplefs_sb_info), GFP_KERNEL);
//     if (!sbi) {
//         ret = -ENOMEM;
//         goto release;
//     }

//     sbi->nr_blocks = csb->nr_blocks;
//     sbi->nr_inodes = csb->nr_inodes;
//     sbi->nr_istore_blocks = csb->nr_istore_blocks;
//     sbi->nr_ifree_blocks = csb->nr_ifree_blocks;
//     sbi->nr_bfree_blocks = csb->nr_bfree_blocks;
//     sbi->nr_free_inodes = csb->nr_free_inodes;
//     sbi->nr_free_blocks = csb->nr_free_blocks;
//     sb->s_fs_info = sbi;

//     brelse(bh);

//     /* Alloc and copy ifree_bitmap */
//     sbi->ifree_bitmap =
//         kzalloc(sbi->nr_ifree_blocks * SIMPLEFS_BLOCK_SIZE, GFP_KERNEL);
//     if (!sbi->ifree_bitmap) {
//         ret = -ENOMEM;
//         goto free_sbi;
//     }

//     for (i = 0; i < sbi->nr_ifree_blocks; i++) {
//         int idx = sbi->nr_istore_blocks + i + 1;

//         bh = sb_bread(sb, idx);
//         if (!bh) {
//             ret = -EIO;
//             goto free_ifree;
//         }

//         memcpy((void *) sbi->ifree_bitmap + i * SIMPLEFS_BLOCK_SIZE, bh->b_data,
//                SIMPLEFS_BLOCK_SIZE);

//         brelse(bh);
//     }

//     /* Alloc and copy bfree_bitmap */
//     sbi->bfree_bitmap =
//         kzalloc(sbi->nr_bfree_blocks * SIMPLEFS_BLOCK_SIZE, GFP_KERNEL);
//     if (!sbi->bfree_bitmap) {
//         ret = -ENOMEM;
//         goto free_ifree;
//     }

//     for (i = 0; i < sbi->nr_bfree_blocks; i++) {
//         int idx = sbi->nr_istore_blocks + sbi->nr_ifree_blocks + i + 1;

//         bh = sb_bread(sb, idx);
//         if (!bh) {
//             ret = -EIO;
//             goto free_bfree;
//         }

//         memcpy((void *) sbi->bfree_bitmap + i * SIMPLEFS_BLOCK_SIZE, bh->b_data,
//                SIMPLEFS_BLOCK_SIZE);

//         brelse(bh);
//     }

//     /* Create root inode */
//     root_inode = simplefs_iget(sb, 0);
//     if (IS_ERR(root_inode)) {
//         ret = PTR_ERR(root_inode);
//         goto free_bfree;
//     }
// #if USER_NS_REQUIRED()
//     inode_init_owner(&init_user_ns, root_inode, NULL, root_inode->i_mode);
// #else
//     inode_init_owner(root_inode, NULL, root_inode->i_mode);
// #endif
    
//     sb->s_root = d_make_root(root_inode);
//     if (!sb->s_root) {
//         ret = -ENOMEM;
//         goto iput;
//     }

//     return 0;

iput:
    iput(root_inode);
free_bfree:
    kfree(sbi->bfree_bitmap);
free_ifree:
    kfree(sbi->ifree_bitmap);
free_sbi:
    kfree(sbi);
release:
    brelse(bh);

    return ret;
}


struct dentry *simplefs_mount(struct file_system_type *fs_type,
                              int flags,
                              const char *dev_name,
                              void *data)
{
    struct dentry *dentry = mount_bdev(fs_type, flags, dev_name, data, simplefs_fill_super);
    if (IS_ERR(dentry))
        pr_err("'%s' mount failure\n", dev_name);
    else
        pr_info("'%s' mount success\n", dev_name);

    return dentry;
}


static struct file_system_type simplefs_file_system_type = {
    .owner = THIS_MODULE,
    .name = "simplefs",
    .mount = simplefs_mount,
    // .kill_sb = simplefs_kill_sb,
    .fs_flags = FS_REQUIRES_DEV,
    .next = NULL,
};

static int __init simplefs_init(void)
{
    int ret = register_filesystem(&simplefs_file_system_type);
    if (ret) {
        pr_err("register_filesystem() failed\n");
        goto end;
    }

    pr_info("module loaded\n");
end:
    return ret;
}

static void __exit simplefs_exit(void)
{
    int ret = unregister_filesystem(&simplefs_file_system_type);
    if (ret)
        pr_err("unregister_filesystem() failed\n");

    pr_info("module unloaded\n");
}



module_init(simplefs_init);
module_exit(simplefs_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("a simple file system");