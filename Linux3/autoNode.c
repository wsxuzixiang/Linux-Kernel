/* 让Linux在驱动加载之后自动生成设备节点 */
/*
class_create()  // 创建类，会在"/sys/class/"下创建文件夹
class_destory()  // 移除创建的类
device_create()  // 在创建的类下创建设备
device_destory()  // 移除创建的设备
*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#define HELLO_MAJOR 0
#define HELLO_NR_DEVS 2

int hello_major = HELLO_MAJOR;
int hello_minor = 0;
int hello_nr_devs = HELLO_NR_DEVS;

dev_t devt;  /* 高12位是主设备号， 低20位是次设备号 */

module_param(hello_major, int, S_IRUGO);
module_param(hello_minor, int, S_IRUGO);
module_param(hello_nr_devs, int, S_IRUGO);

struct hello_char_dev{
    struct cdev cdev;
    char c;
};

struct hello_char_dev *hc_devp;
struct class *hc_cls;

int hc_open(struct inode *inode, struct file *filp){
    printk(KERN_INFO "open hc_dev%d %d\n", iminor(inode), MINOR(inode->i_cdev->dev));
    return 0;
}
ssize_t hc_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
    printk(KERN_INFO "read hc_dev\n");
    return 0;
}
ssize_t hc_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
    printk(KERN_INFO "write hc_dev\n");
    return count;
}
int hc_release(struct inode *inode, struct file *filp){
    printk(KERN_INFO "release hc_dev\n");
    return 0;
}

/* 字符设备操作函数 */
struct file_operations hc_fops = {
    .owner = THIS_MODULE,
    .read = hc_read,
    .write = hc_write,
    .open = hc_open,
    .release = hc_release,
};

/* 模块初始化函数 */
static int __init hello_init(void){
    int ret = 0, i = 0;

    printk(KERN_INFO "---BEGIN HELLO LINUX MODEULE---\n");
    if (hello_major){
        devt = MKDEV(hello_major, hello_minor);
        ret = register_chrdev_region(devt, hello_nr_devs, "hello_chr");
    }
    else{
        ret = alloc_chrdev_region(&devt, hello_minor, hello_nr_devs, "hello_chr");
        hello_major = MAJOR(devt);
    }

    if (ret < 0){
        printk(KERN_WARNING "Sorry: Can't get major %d\n", hello_major);
        goto fail;
    }
    else{
        ;
    }

    hc_devp = kzalloc(sizeof(struct hello_char_dev) * hello_nr_devs, GFP_KERNEL);
    if (!hc_devp){
        printk(KERN_WARNING "alloc mem failed");
        ret = -ENOMEM;
        goto failure_kzalloc;
    }
    for (i = 0; i < hello_nr_devs; i++){
        cdev_init(&hc_devp[i].cdev, &hc_fops);
        hc_devp[i].cdev.owner = THIS_MODULE;
        ret = cdev_add(&hc_devp[i].cdev, MKDEV(hello_major, hello_minor + i), 1);
        if (ret){
        printk(KERN_WARNING "fail add hc_dev%d", i);
        }
    }
    hc_cls = class_create(THIS_MODULE, "hc_dev");
    if(!hc_cls){
        printk(KERN_WARNING "fail create class");
        ret = PTR_ERR(hc_cls);
        goto failure_class;
    }
    for (i = 0; i < hello_nr_devs; i++){
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

static void __exit hello_exit(void){
    int i = 0;

    for (i = 0; i < hello_nr_devs; i++){
        device_destroy(hc_cls, MKDEV(hello_major, hello_minor + i));
    }
    class_destroy(hc_cls);
    for (i = 0; i < hello_nr_devs; i++){
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
MODULE_VERSION("V2.0");