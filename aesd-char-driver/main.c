/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include <linux/mutex.h>
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Your Name Here"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    struct aesd_circular_buffer *buffer = dev->buffer;
    kfree((buffer->entry[buffer->in_offs]).buffptr);

    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    struct aesd_circular_buffer *buffer = dev->buffer;
    struct mutex *lock = &(dev->lock);

    char *kbuf = kmalloc(count, GFP_KERNEL);
    if(kbuf == NULL){
        return -ENOMEM;
    }

    mutex_lock(lock);
    retval = aesd_circular_buffer_raed(buffer, kbuf, count);
    mutex_unlock(lock);
    if(copy_to_user(buf, kbuf, count)){
        return -ENOMEM;
    }

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_buffer_entry entry;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    if(dev == NULL){
        PDEBUG("write(): dev is NULL");
        return -ENOMEM;
    }
    struct aesd_circular_buffer *buffer = dev->buffer;
    struct mutex *lock = &(dev->lock);
    if(buffer == NULL){
        PDEBUG("write(): dev->buffer is null");
        return -ENOMEM;
    }

     /* set up new entry */
     PDEBUG("write(): alloc buffer");
     char *kbuf = kmalloc(count, GFP_KERNEL);
     if(kbuf == NULL){
        PDEBUG("write(): failed for kmalloc");
        return -ENOMEM;
     }

     entry.buffptr = kbuf;

     /* copy to the new entry */
     PDEBUG("write(): start copy from user");
#if 0
     if(copy_from_user(kbuf, buf, count)){
        retval = -ENOMEM;
        PDEBUG("write(): failed for copy_from_user");
        return -ENOMEM;
     }
     entry.size = count;

     mutex_lock(lock);
         
     /* free if already allocated */
     if(buffer->full){
        kfree((buffer->entry[buffer->in_offs]).buffptr);
     }

     /* add to circular buffer */
     PDEBUG("write(): start write to circular buffer");
    aesd_circular_buffer_add_entry(buffer, &entry);
    retval = count;

    mutex_unlock(lock);
#endif
    retval = count;
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_device.buffer = kmalloc(sizeof(struct aesd_circular_buffer), GFP_KERNEL);
    if(aesd_device.buffer == NULL){
        return -ENOMEM;
    }
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
