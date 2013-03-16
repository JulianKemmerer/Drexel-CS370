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
#include <linux/syscalls.h>
#include <linux/fcntl.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Julian Kemmerer");

//Not sure what this does...
#define TARFS_MAGIC 0x19980122

//Tar file header
struct posix_header 
{                              /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
};

//This will be the private data stored in the file
#define MAX_FILE_TARFS_FILE_SIZE 1000
struct tar_file {
    struct posix_header header;
    char  contents[MAX_FILE_TARFS_FILE_SIZE];
    int content_end;
    int is_active;
} typedef tar_file;

//Struct representing a tar file system
#define MAX_FILES_PER_FS_INSTANCE 50
#define MAX_TAR_FILE_PATH_LENGTH 100
struct tarfs
{
	char tar_filepath[MAX_TAR_FILE_PATH_LENGTH];
	int is_active;
	tar_file files[MAX_FILES_PER_FS_INSTANCE];
	int file_count;
};

//List of tarfs instances
#define MAX_TARFS_INSTANCES 50
//List of instances
struct tarfs tarfs_instances[MAX_TARFS_INSTANCES];

//Terrible terrible hack
//Use a temp global tarfs instance ptr to search for tarfs instance between
//Function calls of get_super and fill_super
struct tarfs * tmp_tarfs_ptr;

//Add instance return pointer to new instance
struct tarfs * add_tarfs_instance(const char *devname)
{
	printk(KERN_DEBUG "Adding instance of tarfs: %s\n",devname);
	
	//Loop through list of instances
	//And check for one that is not active
	//Use that space (lazy, I know)
	int i;
	for(i=0; i< MAX_TARFS_INSTANCES; i++)
	{
		if(tarfs_instances[i].is_active != 1)
		{
			//Found an instance
			//TODO
			//Add filepath into struct
			strcpy(tarfs_instances[i].tar_filepath,devname);
			return &(tarfs_instances[i]);
		}
	}
	//Done, found nothing
	return NULL;
}

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

void add_tarfs_file(struct tarfs * tfs, tar_file tarfile,struct super_block *sb, struct dentry *root)
{
	//Update files list
	tfs->files[tfs->file_count] = tarfile;
	tfs->file_count++;
	
	//Add this as a vfs entry
	//printk(KERN_DEBUG "Creating tarfs file with name: %s\n", tarfile.header.name);
	tarfs_create_file(sb, root, tfs->files[tfs->file_count].header.name);
	
	//Examples
	/*
	//Declare subdir
	struct dentry *subdir;

	//Create top level dir
	
	//Create sub dir
	subdir = tarfs_create_dir(sb, root, "subdir");
	if (subdir)
		tarfs_create_file(sb, subdir, "subdirfile");
		*/
}


//Populate a tarfs instance
void process_tar_file(struct tarfs * tarfs_instance,struct super_block *sb, struct dentry *root)
{
	//Start reading from the tar file
	int fd;
	//Single char buffer
	char c;
	//Filename from instance
	char * filename = tarfs_instance->tar_filepath;
	//printk(KERN_DEBUG "Starting tarfs instance from file: %s\n", filename);
	//File count
	int offset = 0;
	int header_offset = 500;
	//c is current character being read
	int count_nulls = 0;
	int file_started = 0;
	//Tmp header to hold info during iterations
	struct posix_header header;

	//See this article here before demanding more comments
	//http://www.linuxjournal.com/article/8110?page=0,1
	mm_segment_t old_fs = get_fs();
	set_fs(KERNEL_DS);

	//Open the file
	fd = sys_open(filename, O_RDONLY, 0);
	if (fd >= 0) 
	{
		while (sys_read(fd,&c, sizeof(char)) == sizeof(char))
		{
			//One char at a time
			//Now use Tim's code...
			//Get all the meta data for the files and the content
			if(c != -1) //EOF == -1?
			{ 
				//Adjust null count
				if(!c) 
				{
					// If we have a null character, we need to increment the number of nulls.
					// This will get reset each time we hit a character that is not a null
					count_nulls++;
					
					if(count_nulls >= 493) { // This is basically the "magic number" of nulls we need to declare the end of a file's contents
					// We are now resetting the loop
					offset = 0;
					file_started = 0;
					//Don't need to unget since this executes right after increment
					}
					
					//Do skip this iteration for this null char though
					continue;
				} 
				
				//Must be a non null char
				count_nulls = 0;
					
				//Do rest of normal stuff		
				
				//Check for stage of reading...
				if(offset < header_offset) {
					// Load in the headers based on their offset 
					if(offset < 100) {
						file_started = 1;
						header.name[offset] = c;
					}
					else if(offset < 108)
						header.mode[offset - 100] = c;
					else if(offset < 116)
						header.uid[offset - 108] = c;
					else if(offset < 124)
						header.gid[offset - 116] =  c;
					else if(offset < 136)
						header.size[offset - 124] = c;
					else if(offset < 148)
						header.mtime[offset - 136] = c;
					else if(offset < 156)
						header.chksum[offset - 148] = c;
					else if(offset < 157)
						header.typeflag = c;
					else if(offset < 257)
						header.linkname[offset - 157] = c;
					else if(offset < 263)
					   header.magic[offset - 257] = c;
					else if(offset < 265)
						header.version[offset - 263] = c;
					else if(offset < 297)
						header.uname[offset - 265] = c;
					else if(offset < 329)
						header.gname[offset - 297] = c;
					else if(offset < 337)
						header.devmajor[offset - 329] = c;
					else if(offset < 345)
						header.devminor[offset - 337] = c;
					else
						header.prefix[offset - 345] = c;
				}
				else if (offset == header_offset){
					// We've completed one entire file header
					//Populate a file struct
					tar_file f;
					f.header = header;
					f.content_end = 0;
					//Take care of adding this at a file
					add_tarfs_file(tarfs_instance, f,sb,root);
					//Reset header
					struct posix_header reset;
					header = reset;
				} else { 
					// Since we've completed a file header, the rest must be the file's content until we reach enough of the null delimiter
					if(c){
						// Read the character into the file's contents
						tar_file * f = &(tarfs_instance->files[tarfs_instance->file_count-1]);
						if(f->content_end < MAX_FILE_TARFS_FILE_SIZE) {
							// As long as we have room
							f->contents[f->content_end] = c;
							f->content_end++;
						}
					}
				}
				
				//Increment offset
				if(file_started == 1)
					offset++;
			}
			//EOF reached
		}
		sys_close(fd);
	}
	set_fs(old_fs);	
		
}


//Create files
static void tarfs_create_files (struct super_block *sb, struct dentry *root)
{
	//Ok we need to read the file
	//File name is dev name stored in sb private data
	//TODO process the sb
	struct tarfs * tarfs_instance;
	tarfs_instance = (struct tarfs *)sb->s_fs_info;
	process_tar_file(tarfs_instance,sb,root);
}

//Generic operations from already written functions in the kernel
static struct super_operations tarfs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

void init_tarfs_struct(struct tarfs * tfs)
{
	//Init values
	tfs->is_active = 1;
	tfs->file_count = 0;
}

//Fill super block (root directory)
static int tarfs_fill_super (struct super_block *sb, void *data, int silent)
{
	//Give the super_block private information regarding this instance
	//Use tmp ptr as not condoned in declaration, then init
	sb->s_fs_info = tmp_tarfs_ptr;
	init_tarfs_struct( (struct tarfs *)sb->s_fs_info);
	
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
	//Feel like this is a hack (see above notes too)
	//Add a new tarfs instance with the devname
	//Then in tarfs_fill_super use this to search for our instance
	tmp_tarfs_ptr = add_tarfs_instance(devname);
	
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
