//Based on filesystem module example found here
//http://lwn.net/Articles/57371/
//And the general guide found here
//http://lwn.net/Articles/57369/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	
#include <linux/fs.h>     	
#include <asm/atomic.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Julian Kemmerer");

#define TARFS_MAGIC 0x19980122


//Function to convert block size to bits (from example)
static inline unsigned int blksize_bits(unsigned int size)
{
	unsigned int bits = 8;
	do {
	 bits++;
	 size >>= 1;
	} while (size > 256);
	return bits;
}


//Need to make an inode to represent file internally
static struct inode * tarfs_make_inode(struct super_block *sb, int mode)
{
	struct inode *ret = new_inode(sb);
	if (ret) {
		ret->i_mode = mode;
		ret->i_uid = ret->i_gid = 0;
		ret->i_blkbits = blksize_bits(PAGE_CACHE_SIZE);
		ret->i_blocks = 0;
		ret->i_atime = ret->i_mtime = ret->i_ctime = CURRENT_TIME;
	}
	return ret;
}

//File operations
//Open a file
static int tarfs_open(struct inode *inode, struct file *filp)
{
	//Copy over information specific to file
	filp->private_data = inode->i_private;
	return 0;
}

#define TMPSIZE 20

//Read a file
static ssize_t tarfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	//Read the file specific data
	//filp->private_data;
	
	//Print data back to user by doing
	//Use the offset value to read from specific area in file
	char tmp[TMPSIZE];
	int len_of_read = 0;
	//len_of_read = snprintf(tmp, TMPSIZE, "%d\n", v);
	
	//Keep track of how many bytes read and increment offset at end
	if (*offset > len_of_read)
		return 0;
	if (count > len_of_read - *offset)
		count = len_of_read - *offset;

	//Copy to user space
	if (copy_to_user(buf, tmp + *offset, count))
		return -EFAULT;
	
	//Increment offset
	*offset += count;
	return count;
}

//Write to a file
static ssize_t tarfs_write_file(struct file *filp, const char *buf,
		size_t count, loff_t *offset)
{
	//Get private data using
	//filp->private_data;
	char tmp[TMPSIZE];
	
	//Read value from user
	if (count >= TMPSIZE)
		return -EINVAL;
	//Set all zeros to start	
	memset(tmp, 0, TMPSIZE);
	//Copy in from user
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
	
	//Modify the private data
	//TODO
	
	//Return bytes written
	return count;
}


//Put together file operations into struct
static struct file_operations tarfs_file_ops = {
	.open	= tarfs_open,
	.read 	= tarfs_read_file,
	.write  = tarfs_write_file,
};


//Create a file
static struct dentry *tarfs_create_file (struct super_block *sb,
		struct dentry *dir, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	//Hash the name and store it
	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);

	//Create directory entry and inode to go with it
	dentry = d_alloc(dir, &qname);
	if (! dentry)
		goto out;
	inode = tarfs_make_inode(sb, S_IFREG | 0644);
	if (! inode)
		goto out_dput;
	inode->i_fop = &tarfs_file_ops;
	//File specific info
	//inode->i_private = counter;

	//Place inode into directory cache
	d_add(dentry, inode);
	return dentry;

	//Errors
	out_dput:
		dput(dentry);
	out:
		return 0;
}

//Directory create
//Similar to file create but with different create mode
static struct dentry *tarfs_create_dir (struct super_block *sb,
		struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name = name;
	qname.len = strlen (name);
	qname.hash = full_name_hash(name, qname.len);
	dentry = d_alloc(parent, &qname);
	if (! dentry)
		goto out;

	inode = tarfs_make_inode(sb, S_IFDIR | 0644);
	if (! inode)
		goto out_dput;
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;
}


//Create files
static void tarfs_create_files (struct super_block *sb, struct dentry *root)
{
	//Declare subdir
	struct dentry *subdir;

	//Create top level dir
	tarfs_create_file(sb, root, "topleveldir");

	//Create sub dir
	subdir = tarfs_create_dir(sb, root, "subdir");
	if (subdir)
		tarfs_create_file(sb, subdir, "subdirfile");
}

//Generic operations from already written functions in the kernel
static struct super_operations tarfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

//Fill super block for root directory
static int tarfs_fill_super (struct super_block *sb, void *data, int silent)
{
	//Root inode and directory entry
	struct inode *root;
	struct dentry *root_dentry;

	//Params for filesystem
	sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = TARFS_MAGIC;
	sb->s_op = &tarfs_s_ops;

	//Make root directory inode
	root = tarfs_make_inode (sb, S_IFDIR | 0755);
	if (! root)
		goto out;
	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	//Put dentry into directory cache so vfs can find it
	root_dentry = d_alloc_root(root);
	if (! root_dentry)
		goto out_iput;
	sb->s_root = root_dentry;

	//Call the function to populate the root directory with files
	tarfs_create_files (sb, root_dentry);
	return 0;
	
	//Errors
	out_iput:
		iput(root);
	out:
		return -ENOMEM;
}

//Stuff to pass in when registering the filesystem
static int tarfs_get_super(struct file_system_type *fst,
	 int flags, const char *devname, void *data,
	 struct vfsmount *mnt)
{
	//This is called when filesystem is mounted
	//Also calls tarfs_fill_super
	return get_sb_single(fst, flags, data, tarfs_fill_super, mnt);
}

//More file system info
static struct file_system_type tarfs_type = {
	.owner 		= THIS_MODULE,
	.name		= "tarfs",
	.get_sb		= tarfs_get_super,
	.kill_sb	= kill_litter_super,
};

//Module setup
static int __init tarfs_init(void)
{
	return register_filesystem(&tarfs_type);
}
//Module take down
static void __exit tarfs_exit(void)
{
	unregister_filesystem(&tarfs_type);
}
module_init(tarfs_init);
module_exit(tarfs_exit);
