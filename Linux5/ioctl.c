#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/cdev.h>

#include "hello_chr_locked.h"

#define HELLO_MAJOR 0
#define HELLO_NR_DEVS 2

int hello_major = HELLO_MAJOR;
int hello_minor = 0;
int hello_nr_devs = HELLO_NR_DEVS;
dev_t devt;

module_param(hello_major, int, S_IRUGO);
module_param(hello_minor, int ,S_IRUGO);
module_param(hello_nr_devs, int, S_IRUGO);

struct hello_char_dev{
    struct cdev cdev;
    char *c;  /* 保存字符串 */
    int n;  /* 保存字符个数 */
};

struct hello_char_dev *hc_devp;
struct class *hc_cls;

/* open函数的主要作用是获得字符设备结构体的地址，并将地址保存起来 */
int hc_open(struct inode *inode, struct file *filp)
{
    struct hello_char_dev *hc_dev;

    printk(KERN_INFO "open hc_dev%d %d\n", iminor(inode), MINOR(inode->i_cdev->dev));

    /* 获取设备结构体的地址 */
    hc_dev = container_of(inode->i_cdev, struct hello_char_dev, cdev);

    /* 将设备结构地址放到文件描述符结构的私有数据中 */
    filp->private_data = hc_dev;

    return 0;
}

/* 将字符设备中的字符串给用户 */
ssize_t hc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    /* 获得字符设备的地址 ，保存在hc_dev中 */
    struct hello_char_dev *hc_dev = filp->private_data;

    printk(KERN_INFO "read hc_dev %p]n", hc_dev);

    /* 数据的偏移量是否大于数据的总量 */
    if (*f_pos >= hc_dev->n) {
        goto out;
    }

    /* 所需要读取个数和要读取数据之和是否大于数据总量*/
    if (*f_pos + count > hc_dev->n) {
        count = hc_dev->n - *f_pos;
    }

    if (copy_to_user(buf, hc_dev->c, count)) {
        retval = -EFAULT;
        goto out;
    }

    *f_pos += count;

    return count;

out:
    return retval;
}

ssize_t hc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct hello_char_dev *hc_dev = filp->private_data;
    int retval = -ENOMEM;

    kfree(hc_dev->c);
    hc_dev->c = NULL;
    hc_dev->n = 0;
    hc_dev->c = kzalloc(count, GFP_KERNEL);

    if (!hc_dev->c) {
        goto out;
    }

    if (copy_from_user(hc_dev->c, buf, count)) {
        retval = -EFAULT;
        goto fail_copy;
    }

    hc_dev->n = count;

    return count;

fail_copy:
    kfree(hc_dev->c);
out:
    return retval;
}

int hc_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "release hc_dev\n");
    return 0;
}

long hc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct hello_char_dev *hc_dev = filp->private_data;
	long retval = 0;
	int tmp,err=0;
	
    /* 检查幻数 */
	if (_IOC_TYPE(cmd) != HC_IOC_MAGIC) 
        return -ENOTTY;

    /* 检查命令编号 */
	if (_IOC_NR(cmd) > HC_IOC_MAXNR) 
        return -ENOTTY;	
	
    /* 检查用户空间地址是否是用 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) 
        return -EFAULT;
	
	switch(cmd){
		case HC_IOC_RESET:
			printk(KERN_INFO "ioctl reset\n");
			kfree(hc_dev->c);
			hc_dev->n=0;
			break;
		case HC_IOCP_GET_LENS:
			printk(KERN_INFO "ioctl get lens through pointer\n");
			retval = __put_user(hc_dev->n,(int __user *)arg);
			break;
		case HC_IOCV_GET_LENS:
			printk(KERN_INFO "ioctl get lens through value\n");
			return hc_dev->n;
			break;
		case HC_IOCP_SET_LENS:
			printk(KERN_INFO "ioctl set lens through pointer");
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;			
			retval = get_user(tmp,(int __user *)arg);			
			if(hc_dev->n>tmp)
				hc_dev->n=tmp;
			printk(KERN_INFO " %d\n",hc_dev->n);
			break;
		case HC_IOCV_SET_LENS:
			printk(KERN_INFO "ioctl set lens through value");
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;			
			hc_dev->n = min(hc_dev->n,(int)arg);
			printk(KERN_INFO " %d\n",hc_dev->n);
			break;
		default:
			break;
	}
	
	return retval;
}

struct file_operations hc_fops = {
    .owner = THIS_MODULE,
    .read = hc_read,
    .write = hc_write,
    .open = hc_open,
    .release = hc_release,
    .unlocked_ioctl = hc_ioctl,
};

/* 模块初始化函数 */
static int __init hello_init(void)
{
    int ret = 0, i = 0;

    printk(KERN_INFO "---BEGIN HELLO LINUX MODEULE---\n");

    if (hello_major) {
        devt = MKDEV(hello_major, hello_minor);
        ret = register_chrdev_region(devt, hello_nr_devs, "hello_chr");
    }
    else{
        ret = alloc_chrdev_region(&devt, hello_minor, hello_nr_devs, "hello_chr");
        hello_major = MAJOR(devt);
    }

    if (ret < 0) {
        printk(KERN_WARNING "Sorry: Can't get major %d\n", hello_major);
        goto fail;
    }

    hc_devp = kzalloc(sizeof(struct hello_char_dev) * hello_nr_devs, GFP_KERNEL);

    if (!hc_devp) {
        printk(KERN_WARNING "alloc mem failed");
        ret = -ENOMEM;
        goto failure_kzalloc;
    }

    for (i = 0; i < hello_nr_devs; i++) {
        cdev_init(&hc_devp[i].cdev, &hc_fops);
        hc_devp[i].cdev.owner = THIS_MODULE;

        ret = cdev_add(&hc_devp[i].cdev, MKDEV(hello_major, hello_minor + i), 1);

        if (ret) {
        printk(KERN_WARNING "fail add hc_dev%d", i);
        }
    }

    hc_cls = class_create(THIS_MODULE, "hc_dev");

    if (!hc_cls) {
        printk(KERN_WARNING "fail create class");
        ret = PTR_ERR(hc_cls);
        goto failure_class;
    }

    for (i = 0; i < hello_nr_devs; i++) {
        device_create(hc_cls, NULL, MKDEV(hello_major, hello_minor + i), NULL, "hc_dev%d", i);
    }

    printk(KERN_INFO "---END HELLO LINUX MODULE---\n");

    return 0;

failure_class:
    kfree(hc_devp);

failure_kzalloc:
    unregister_chrdev_region(devt, hello_nr_devs);

fail:
    return ret;
}

static void __exit hello_exit(void)
{
    int i = 0;

    for (i = 0; i < hello_nr_devs; i++) {
        device_destroy(hc_cls, MKDEV(hello_major, hello_minor + i));
    }

    class_destroy(hc_cls);

    for (i = 0; i < hello_nr_devs; i++) {
        cdev_del(&hc_devp[i].cdev);
    }

    kfree(hc_devp);
    unregister_chrdev_region(devt, hello_nr_devs);
    printk(KERN_INFO "GOODBYE LINUX\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("XZX");
MODULE_VERSION("V1.0");