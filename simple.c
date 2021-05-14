#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define SIMPLE_MAX_DEVICES	1
#define FIRST_MINOR		0
#define NELEM			256

/* You should really check out drivers/char/bsr.c 
 * for pretty simple character device */
struct simple {
	char		*data;
	ssize_t		nobytes;	/* no of bytes written to data */
	struct cdev	smpl_cdev;
	struct device	*smpl_device;
	struct mutex	io_mutex;
};

struct simple *simple_dev;
static struct class *smpl_class;
static dev_t devt;

int simple_open(struct inode *inode, struct file *filp)
{
	struct simple *dev;
	dev = container_of(inode->i_cdev, struct simple, smpl_cdev);
	filp -> private_data = dev;

	return 0;
}

ssize_t simple_read(struct file *filp, char __user *buffer, size_t count,
		loff_t *f_pos)
{
	struct simple *dev = filp -> private_data;
	ssize_t retval = 0 ;

	retval = mutex_trylock(&dev->io_mutex);
	if (retval == 0) { /* mutex has not been aquired */
		if (filp->f_flags & O_NONBLOCK)	
			return -EAGAIN;		/* non-blocking write. */

		retval = mutex_lock_interruptible(&dev->io_mutex);
		if (retval < 0)
			return retval;	/* signal recieved */
	}

	if(count > dev->nobytes)
		count = dev->nobytes;

	printk(KERN_ALERT "bytes to read %ld\n", count);

	if(copy_to_user(buffer, dev->data, count)) {
		retval = -EFAULT;
		goto out;
	}
	retval = count;
	dev->nobytes -= count;
out:
	mutex_unlock(&dev->io_mutex);
	return retval;

}

ssize_t simple_write(struct file *filp, const char __user *buffer, size_t count,
		loff_t *f_pos)
{
	struct simple *dev = filp -> private_data;
	ssize_t retval = -ENOMEM;
	
	if(!dev) {
		printk(KERN_ALERT "filp -> private_data == NULL\n");
		return -ENOMEM;
	}

	retval = mutex_trylock(&dev->io_mutex);
	if (retval == 0) { /* mutex has not been aquired */
		if (filp->f_flags & O_NONBLOCK)	
			return -EAGAIN;		/* non-blocking write. */

		retval = mutex_lock_interruptible(&dev->io_mutex);
		if (retval < 0)
			return retval;	/* signal recieved */
	}

	if(count > NELEM) 
	    count = NELEM;

	if(copy_from_user(dev->data, buffer, count)) {
		retval = -EFAULT;
		goto out;
	}

	printk(KERN_ALERT "dev->data: %s\n", dev->data);
	dev -> nobytes = retval = count;

out:
	mutex_unlock(&dev->io_mutex);
	return retval;
}

int simple_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations fops_simple = {
	.owner		= THIS_MODULE,
	.read		= simple_read,
	.write		= simple_write,
	.open		= simple_open,
	.release	= simple_release
};

static int __init simple_init(void)
{
	int ret;
	simple_dev = kzalloc(sizeof(struct simple), GFP_KERNEL);
	if (!simple_dev) 
		return -ENOMEM;

	simple_dev -> data = kmalloc_array(NELEM, sizeof(char), GFP_KERNEL);
	if (!simple_dev->data) { 
		ret = -ENOMEM;
		goto fail_1;
	}

	mutex_init(&simple_dev->io_mutex);
	simple_dev -> nobytes = 0; 

	ret = alloc_chrdev_region(&devt, FIRST_MINOR, SIMPLE_MAX_DEVICES, "simple");
	if (ret < 0) 
		goto fail_2;

	smpl_class = class_create(THIS_MODULE, "simple");
	if (IS_ERR(smpl_class)) {
		ret = PTR_ERR(smpl_class);
		goto fail_3;
	}

	simple_dev->smpl_device = device_create(smpl_class, NULL, devt, NULL, "simple");
	if (IS_ERR(simple_dev->smpl_device)) {
		ret = PTR_ERR(simple_dev->smpl_device);
		goto fail_4;
	}

	cdev_init(&simple_dev->smpl_cdev, &fops_simple);
	ret = cdev_add(&simple_dev->smpl_cdev, devt, SIMPLE_MAX_DEVICES);
	if (ret < 0)
		goto fail_5;

	return 0;

fail_5:
	device_destroy(smpl_class, devt);
fail_4:
	class_destroy(smpl_class);
fail_3:
	unregister_chrdev_region(devt, SIMPLE_MAX_DEVICES);
fail_2:
	kfree(simple_dev->data);
fail_1:
	kfree(simple_dev);

	return ret;
}

static void simple_exit(void) 
{
	cdev_del(&simple_dev->smpl_cdev);
	device_destroy(smpl_class, devt);
	kfree(simple_dev->data);
	kfree(simple_dev);
	class_destroy(smpl_class);
	unregister_chrdev_region(devt, 1);
}

MODULE_LICENSE("GPL");
module_init(simple_init);
module_exit(simple_exit);
