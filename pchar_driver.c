#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

// psuedo device
#define SIZE 32
static char buffer[SIZE];

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
    return 0; // module initialized successfully.
}


// module de-initialization function
static void __exit pchar_exit(void) {
    pr_info("%s: pchar_exit() called.\n", THIS_MODULE->name);
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
    int bytes_avail, bytes_toread, nbytes;
    pr_info("%s: pchar_read() called.\n", THIS_MODULE->name);
    // find the bytes available for reading
    bytes_avail = SIZE - *poffset;
    // decide num of bytes to be read (min of avail bytes and user buf size)
    bytes_toread = bytes_avail < bufsize ? bytes_avail : bufsize;
    pr_info("%s: pchar_read() - avail bytes: %d, user buf size: %lu, bytes to read: %d.\n", THIS_MODULE->name, bytes_avail, bufsize, bytes_toread);
    // if num of bytes to read is zero, return end of device (0 bytes)
    if(bytes_toread <= 0) {
        pr_err("%s: pchar_read() end of device reached.\n", THIS_MODULE->name);
        return 0;
    }
    // copy bytes from device buffer to user buffer
    // and calculate number of bytes copied
    nbytes = bytes_toread - copy_to_user(ubuf, buffer + *poffset, bytes_toread);
    pr_info("%s: pchar_read() - bytes read: %d.\n", THIS_MODULE->name, nbytes);
    // update the file position
    *poffset = *poffset + nbytes;
    // returns num of bytes successfully read.
    return nbytes;
}

static ssize_t pchar_write(struct file *pfile, const char __user *ubuf, size_t bufsize, loff_t *poffset) {
    int bytes_avail, bytes_towrite, nbytes;
    pr_info("%s: pchar_write() called.\n", THIS_MODULE->name);
    // find the space avail in buffer
    bytes_avail = SIZE - *poffset;
    // decide num of bytes to be copied from user buffer to device buffer (min of avail device buffer size and user buffer size)
    bytes_towrite = bytes_avail < bufsize ? bytes_avail : bufsize;
    pr_info("%s: pchar_write() - avail bytes: %d, user buf size: %lu, bytes to write: %d.\n", THIS_MODULE->name, bytes_avail, bufsize, bytes_towrite);
    // if no bytes to write i.e. device buffer is full, return 0 or error ENOSPC.
    if(bytes_towrite <= 0) {
        pr_err("%s: pchar_write() no space on device.\n", THIS_MODULE->name);
        return 0; // or return -ENOSPC;
    }
    // copy bytes from user buffer to device buffer (current position onwards)
    // and calculate num of bytes successfully copied.
    nbytes = bytes_towrite - copy_from_user(buffer + *poffset, ubuf, bytes_towrite);
    pr_info("%s: pchar_write() - bytes written: %d.\n", THIS_MODULE->name, nbytes);
    // update the file position
    *poffset = *poffset + nbytes;
    // returns num of bytes successfully written.
    return nbytes;
}

module_init(pchar_init);
module_exit(pchar_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nilesh Ghule <nilesh@sunbeaminfo.com>");
MODULE_DESCRIPTION("Hello Char Device Driver");