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
#include <linux/slab.h>
#include "aesdchar.h"
#include "aesd-circular-buffer.h"
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Your Name Here"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *, struct file *);
int aesd_release(struct inode *, struct file *);
ssize_t aesd_read(struct file *, char __user *, size_t, loff_t *);
ssize_t aesd_write(struct file *, const char __user *, size_t, loff_t *);
int aesd_init_module(void);
void aesd_cleanup_module(void);
static loff_t aesd_llseek(struct file *, loff_t, int);
static long aesd_unlocked_ioctl(struct file *, unsigned int, unsigned long);

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    //filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = &aesd_device;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    // struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    // struct aesd_circular_buffer *buffer = dev->buffer;
    // aesd_circular_buffer_free(buffer);

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
    buffer = aesd_device.buffer;
    struct mutex *lock = &(dev->lock);
    char *tmp_ubuf;
     
    if(buffer == NULL){
        PDEBUG("read(): dev->buffer is null");
        return -ENOMEM;
    }

    PDEBUG("read(): buffer adr = %p start_abs = %d, w_abs = %d, full = %d", buffer, buffer->start_abs, buffer->w_abs, buffer->full);

    char *kbuf = kmalloc(count, GFP_KERNEL);
    if(kbuf == NULL){
        kfree(kbuf);
        PDEBUG("read(): dev->buffer is null");
        return -ENOMEM;
    }
    memset(kbuf, 0, count);

    //mutex_lock(lock);
    PDEBUG("read(): start read from cir buffer");
    retval = aesd_circular_buffer_raed(buffer, kbuf, count, f_pos);
    //mutex_unlock(lock);

    if(retval > 0){
        if(copy_to_user(buf, kbuf, retval)){
            retval = -ENOMEM;
        }
    }
    kfree(kbuf);

    PDEBUG("read(): completed. ret = %d, ppos = %d", retval, *f_pos);

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
    buffer = aesd_device.buffer;
    struct mutex *lock = &(dev->lock);
    if(buffer == NULL){
        PDEBUG("write(): dev->buffer is null");
        return -ENOMEM;
    }
    PDEBUG("write(): buffer adr = %p start_abs = %d, w_abs = %d, full = %d", buffer, buffer->start_abs, buffer->w_abs, buffer->full);

     /* set up new entry */
     PDEBUG("write(): alloc buffer");
     char *kbuf = kmalloc(count, GFP_KERNEL);
     if(kbuf == NULL){
        PDEBUG("write(): failed for kmalloc");
        return -ENOMEM;
     }

     entry.buffptr = kbuf;

     /* copy to the new entry */

     PDEBUG("write(): user buf = %s, count = %zu", buf, count);
     if(buf[count -1 ]== '\n'){
        PDEBUG("write(): the last of user buf is n");
     }
     else{
        PDEBUG("write(): the last of user buf is not n");
     }

    if (!access_ok(buf, count)) {
        PDEBUG("write(): user pointer not accessible");
        kfree(kbuf);
        return -EFAULT;
    }
    memset(kbuf, 0, count);

    PDEBUG("write(): start copy from user");

     if(copy_from_user(kbuf, buf, count)){
        kfree(kbuf);
        PDEBUG("write(): failed for copy_from_user");
        return -ENOMEM;
     }
     entry.size = count;
     //mutex_lock(lock);
         
     /* free if already allocated */
    //  if(buffer->full){
    //     PDEBUG("write(): free old mem");
    //     kfree((buffer->entry[buffer->in_offs]).buffptr);
    //  }

     /* add to circular buffer */
     PDEBUG("write(): start write to circular buffer");
#if 0
    struct aesd_seekto seekto;
    if(aesd_check_special_str(kbuf, count, &seekto.write_cmd, &seekto.write_cmd_offset) == 0){
        aesd_unlocked_ioctl(filp, AESDCHAR_IOCSEEKTO, (unsigned long)&seekto);
    }
    else{
#endif
        aesd_circular_buffer_add_entry(buffer, &entry);
//    }
    kfree(kbuf);

    PDEBUG("write(): buffer after adr = %p inofs = %d, outofs = %d, full = %d", buffer, buffer->in_offs, buffer->out_offs, buffer->full);
    
    retval = count;

    //mutex_unlock(lock);
    return retval;
}
static loff_t aesd_llseek(struct file *file, loff_t off, int whence)
{
    PDEBUG("aesd_llseek(): filp = %d, off = %d, whence = %d", file->f_pos, off, whence);

    struct aesd_dev *dev = (struct aesd_dev *)file->private_data;
    if(dev == NULL){
        PDEBUG("lseek(): dev is NULL");
        return -ENOMEM;
    }
    struct aesd_circular_buffer *buffer = dev->buffer;
    buffer = aesd_device.buffer;
    struct mutex *lock = &(dev->lock);
    if(buffer == NULL){
        PDEBUG("lseek(): dev->buffer is null");
        return -ENOMEM;
    }

    loff_t newpos;

    switch (whence) {
        case SEEK_SET:
            newpos = (loff_t)buffer->start_char_abs + off;
            break;
        case SEEK_CUR:
            newpos = file->f_pos + off;
            break;
        case SEEK_END:
            newpos = (loff_t)buffer->w_char_abs + off;   // dev->size = current length
            break;
        default:
            return -EINVAL;
    }

    file->f_pos = newpos;

    return newpos;
}

static long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    PDEBUG("aesd_unlocked_ioctl(): filp = %d cmd = %d", filp->f_pos, cmd);
    struct aesd_dev *dev = (struct aesd_dev *)filp->private_data;
    if(dev == NULL){
        PDEBUG("ioctl(): dev is NULL");
        return -ENOMEM;
    }
    struct aesd_circular_buffer *buffer = dev->buffer;
    buffer = aesd_device.buffer;
    if(buffer == NULL){
        PDEBUG("ioctl(): dev->buffer is null");
        return -ENOMEM;
    }
    // struct mutex *lock = &(dev->lock);

    if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) {
        PDEBUG("ioctl(): incorrect cmd type");
        return -EINVAL ;
    }
    if (_IOC_NR(cmd)   >  AESDCHAR_IOC_MAXNR) {
        PDEBUG("ioctl(): incorrect cmd num");
        return -EINVAL ;
    }
#if 0
    if (_IOC_DIR(cmd) & _IOC_READ) {
        if (!access_ok((void __user *)arg, _IOC_SIZE(cmd))){
            PDEBUG("ioctl(): cannot access");
            return -EINVAL ;
        }
        if (_IOC_DIR(cmd) & _IOC_WRITE) {
            if (!access_ok((void __user *)arg, _IOC_SIZE(cmd))){
                PDEBUG("ioctl(): cannot access 2");
                return -EINVAL ;
            }
        }
    }
#endif
    
    struct aesd_seekto *seekto = (struct aesd_seekto *)arg;
    PDEBUG("aesd_unlocked_ioctl(): cmd = %d, ofs = %d", seekto->write_cmd, seekto->write_cmd_offset);

    switch (cmd) {
        case AESDCHAR_IOCSEEKTO:
            
            filp->f_pos = aesd_circular_buffer_get_bytes_to_ofs(buffer, seekto->write_cmd, seekto->write_cmd_offset) + buffer->start_char_abs;
            PDEBUG("aesd_unlocked_ioctl(): cmd = %d, ofs = %d", filp->f_pos, buffer->start_char_abs);
            break;

        default:
            return -EINVAL ;
    }

    return 0;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek = aesd_llseek,
    .unlocked_ioctl = aesd_unlocked_ioctl,
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
    if(aesd_circular_buffer_init(aesd_device.buffer) != 0){
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

     kfree(aesd_device.buffer);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
