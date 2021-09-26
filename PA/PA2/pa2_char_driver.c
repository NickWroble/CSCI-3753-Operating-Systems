
#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/slab.h>
#include<linux/uaccess.h>

#define BUFFER_SIZE 1024
#define MAJOR_NUMBER 240
#define DEVICE_NAME "pa2_character_driver"

MODULE_AUTHOR("Nicholas Wroblewski");
MODULE_LICENSE("GPL");
MODULE_INFO(intree, "Y");

static char *device_buffer;
static int num_open;
static int num_close;


ssize_t pa2_char_driver_read (struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	int bytes_read;
	printk(KERN_ALERT "inside %s function\n", __FUNCTION__);
	if (length < 0 || length > BUFFER_SIZE)
		return -1;
	else if (length > BUFFER_SIZE - (*offset + length))  //not engough room in the buffer
		return -1;
	else if (0 > BUFFER_SIZE - (*offset + length))  //trying to access negative addresses
		return -1;
    bytes_read = length - copy_to_user(buffer, device_buffer + *offset, length);
	printk(KERN_ALERT "bytes_read %d\n", bytes_read);
	*offset += bytes_read; //adjust the fp
	return bytes_read;
}



ssize_t pa2_char_driver_write (struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	int bytes_written; //same code as reading
	printk(KERN_ALERT "inside %s function\n",__FUNCTION__);
	if(length > BUFFER_SIZE - (*offset + length))
		return -1;
	else if(length < 0 || length > BUFFER_SIZE)
		return 0;
	else if (0 > BUFFER_SIZE - (*offset + length))  //trying to access negative addresses
		return -1;
	bytes_written = length - copy_from_user(device_buffer + *offset, buffer, length);
	printk(KERN_ALERT "bytes_written %d\n", bytes_written);
	*offset += bytes_written;
	return bytes_written;
}


int pa2_char_driver_open (struct inode *pinode, struct file *pfile)
{
	printk(KERN_ALERT "inside %s function\n",__FUNCTION__);
	num_open++;
	printk(KERN_ALERT "Opened %d times.\n", num_open);
	return 0;
}

int pa2_char_driver_close (struct inode *pinode, struct file *pfile)
{
	printk(KERN_ALERT "inside %s function\n",__FUNCTION__);
	num_close++;
	printk(KERN_ALERT "Closed %d times.\n",num_open);
	return 0;
}

loff_t pa2_char_driver_seek (struct file *pfile, loff_t offset, int whence)
{
	loff_t new_fp;
	printk(KERN_ALERT "inside %s function\n",__FUNCTION__);
	if 		(whence == 0)
		new_fp = offset;
	else if (whence == 1)
		new_fp = pfile->f_pos + offset;
	else if (whence == 2)
		new_fp = BUFFER_SIZE - offset;
	else{
		printk(KERN_ALERT "Bad whence: %d\n", whence);
		return -1;
	}
	if(new_fp > BUFFER_SIZE || new_fp < 0)
		return -1;
	pfile->f_pos = new_fp;
	return new_fp;
}

struct file_operations pa2_char_driver_file_operations = {
	.owner   = THIS_MODULE,
	.open	 = pa2_char_driver_open,
	.read 	 = pa2_char_driver_read,
	.write   = pa2_char_driver_write,
    .release = pa2_char_driver_close,
	.llseek  = pa2_char_driver_seek
};


static int pa2_char_driver_init(void)
{
	/* print to the log file that the init function is called.*/
	/* register the device */
	int error;
	printk(KERN_ALERT "inside %s function\n",__FUNCTION__);
	error  = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &pa2_char_driver_file_operations);
	if(error)
		return error;
	num_open = 0; 
	num_close = 0;
	device_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	return 0;
}

static void pa2_char_driver_exit(void)
{
	printk(KERN_ALERT "inside %s function\n",__FUNCTION__);
	unregister_chrdev(MAJOR_NUMBER, DEVICE_NAME);
	kfree(device_buffer);
}

module_init(pa2_char_driver_init);
module_exit(pa2_char_driver_exit);

