#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>

// psuedo device -- kfifo
#define SIZE 32
// DEL // static char buffer[SIZE];
static struct kfifo buffer;

// function declarations
static int pchar_open(struct inode *pinode, struct file *pfile);
static int pchar_close(struct inode *pinode, struct file *pfile);
static ssize_t pchar_read(struct file *pfile, char __user *ubuf, size_t bufsize, loff_t *poffset);
static ssize_t pchar_write(struct file *pfile, const char __user *ubuf, size_t bufsize, loff_t *poffset);

// global variables
static dev_t devno;
static struct class *pclass;
static struct file_operations pchar_ops = {
    .owner = THIS_MODULE,
    .open = pchar_open,
    .release = pchar_close,
    .read = pchar_read,
    .write = pchar_write
};
static struct cdev pchar_cdev;

// module initialization function
static int __init pchar_init(void) {
    int ret;
    struct device *pdevice;
    pr_info("%s: pchar_init() called.\n", THIS_MODULE->name);
    // allocate device number
    ret = alloc_chrdev_region(&devno, 0, 1, "pchar");
    if(ret < 0) {
        pr_err("%s: alloc_chrdev_region() failed.\n", THIS_MODULE->name);
        return ret; // return error code
    }
    pr_info("%s: device number = %d/%d.\n", THIS_MODULE->name, MAJOR(devno), MINOR(devno));
    // create device class
    pclass = class_create(THIS_MODULE, "pchar_class");
    if(IS_ERR(pclass)) {
        pr_err("%s: class_create() failed.\n", THIS_MODULE->name);
        unregister_chrdev_region(devno, 1);
        return -1; // return error
    }
    pr_info("%s: device class is created.\n", THIS_MODULE->name);
    // create device file
    pdevice = device_create(pclass, NULL, devno, NULL, "pchar");
    if(IS_ERR(pdevice)) {
        pr_err("%s: device_create() failed.\n", THIS_MODULE->name);
        class_destroy(pclass);
        unregister_chrdev_region(devno, 1);
        return -1; // return error
    }
    pr_info("%s: device file is created.\n", THIS_MODULE->name);
    // init cdev struct and add into kernel db
    cdev_init(&pchar_cdev, &pchar_ops);
    ret = cdev_add(&pchar_cdev, devno, 1);
    if(ret < 0) {
        pr_err("%s: cdev_add() failed.\n", THIS_MODULE->name);
        device_destroy(pclass, devno);
        class_destroy(pclass);
        unregister_chrdev_region(devno, 1);
        return ret; // return error code
    }
    pr_info("%s: device cdev is added in kernel.\n", THIS_MODULE->name);
    // allocate device buffer (kfifo)
    ret = kfifo_alloc(&buffer, SIZE, GFP_KERNEL);
    if(ret) {
        pr_err("%s: kfifo_alloc() failed.\n", THIS_MODULE->name);
        cdev_del(&pchar_cdev);
        device_destroy(pclass, devno);
        class_destroy(pclass);
        unregister_chrdev_region(devno, 1);
        return ret;
    }
    pr_info("%s: allocated device buffer of size %d.\n", THIS_MODULE->name, kfifo_size(&buffer));
    return 0; // module initialized successfully.
}


// module de-initialization function
static void __exit pchar_exit(void) {
    pr_info("%s: pchar_exit() called.\n", THIS_MODULE->name);
    // deallocate device buffer (kfifo)
    kfifo_free(&buffer);
    pr_info("%s: device buffer is released.\n", THIS_MODULE->name);
    // remove device cdev from the kernel db.
    cdev_del(&pchar_cdev);
    pr_info("%s: device cdev is removed from kernel.\n", THIS_MODULE->name);
    // destroy device file
    device_destroy(pclass, devno);
    pr_info("%s: device file is destroyed.\n", THIS_MODULE->name);
    // destroy device class
    class_destroy(pclass);
    pr_info("%s: device class is destroyed.\n", THIS_MODULE->name);
    // unallocate device number
    unregister_chrdev_region(devno, 1);
    pr_info("%s: device number released.\n", THIS_MODULE->name);
}

// pchar file operations
static int pchar_open(struct inode *pinode, struct file *pfile) {
    pr_info("%s: pchar_open() called.\n", THIS_MODULE->name);
    return 0;
}

static int pchar_close(struct inode *pinode, struct file *pfile) {
    pr_info("%s: pchar_close() called.\n", THIS_MODULE->name);
    return 0;
}

static ssize_t pchar_read(struct file *pfile, char __user *ubuf, size_t bufsize, loff_t *poffset) {
    int nbytes, ret;
    pr_info("%s: pchar_read() called.\n", THIS_MODULE->name);
    // write data from device buffer (kfifo) to the user buffer
    ret = kfifo_to_user(&buffer, ubuf, bufsize, &nbytes);
    if(ret) {
        pr_err("%s: failed to copy device buffer into user buffer.\n", THIS_MODULE->name);
        return ret;
    }
    pr_info("%s: pchar_read() %d bytes read.\n", THIS_MODULE->name, nbytes);
    // returns num of bytes successfully read.
    return nbytes;
}

static ssize_t pchar_write(struct file *pfile, const char __user *ubuf, size_t bufsize, loff_t *poffset) {
    int nbytes, ret;
    pr_info("%s: pchar_write() called.\n", THIS_MODULE->name);
    // write user buffer data into device "buffer" (kfifo)
    ret = kfifo_from_user(&buffer, ubuf, bufsize, &nbytes);
    if(ret) {
        pr_err("%s: failed to copy user buffer into device buffer.\n", THIS_MODULE->name);
        return ret;
    }
    pr_info("%s: pchar_write() %d bytes written.\n", THIS_MODULE->name, nbytes);
    // returns num of bytes successfully written.
    return nbytes;
}

module_init(pchar_init);
module_exit(pchar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nilesh Ghule <nilesh@sunbeaminfo.com>");
MODULE_DESCRIPTION("Hello Char Device Driver");