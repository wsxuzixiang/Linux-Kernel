/*
kzalloc() / kmalloc()  // 分配内存存储空间
kfree()  // 释放存储空间
cdev_init()  // 初始化字符设备
cdev_add()  // 将字符设备添加到内核中
cdev_del()  // 移除添加到内核中的字符设备
struct file_operations  // 保存操作字符设备的函数
*/


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#define HELLO_MAJOR 0
#define HELLO_NR_DEVS 2

int hello_major = HELLO_MAJOR;
int hello_minor = 0;

dev_t devt;  /*高12位为主设备号，低20位为次设备号*/

int hello_nr_devs = HELLO_NR_DEVS;

module_param(hello_major, int, S_IRUGO);
module_param(hello_minor, int, S_IRUGO);
module_param(hello_nr_devs, int, S_IRUGO);

/* 实际的字符设备结构，类似于面向对象中的继承 */
struct hello_char_dev{
    struct cdev cdev;
    char c;
}

/* 定义指向结构类型的指针，后续会给指针分配存储空间*/
struct hello_char_dev *hc_devp;

/* 为字符设备构建操作函数 */
int hc_open(struct inode *inode, struct file *filp){
    printk(KERN_INFO "open hc_dev%d %d\n", iminor(inode), MINOR(inode->i_cdev->dev));
    return 0;
}
ssize_t hc_read(struct file *flip, char __user *buf, size_t count, loff_t *f_pos){
    printk(KERN_INFO "read hc_dev\n");
    return 0;
}
ssize_t hc_write(struct file *flip, const char __user *buf, size_t count, loff_t *fpos){
    printk(KERN_INFO, "write hc_dev\n");
    return count;  
}
int hc_release(struct inode *inode, struct file *flip){
    printk(KERN_INFO, "release hc_dev\n");
    return 0;
}

/* 将操作函数填充至operations结构体中 */
struct file_operations hc_fops = {
    .owner = THIS_MODULE,
    .read = hc_read,
    .write = hc_write,
    .open = hc_open,
    .release = hc_release,
};

static int __init hello_init(void){
    int ret, i;

    printk(KERN_INFO "---BEGIN HELLO LINUX MODULE---\n");
    if (hello_major){
        devt = MKDEV(hello_major, hello_minor);
        ret = register_chrdev_region(devt, hello,_nr_devs, "hello_chr");
    }
    else{
        ret = alloc_chrdev_region(&devt, hello_minor, hello_nr_devs, "hello_chr"); /* 动态分配主设备号*/
        hello_major = MAJOR(devt);
    }

    if (ret < 0){
        printk(KERN_WARNING "hello: can't get major %d\n", hello_major);
        goto fail;
    }
    hc_devp = kzalloc(sizeof(struct hello_char_dev)*hello_nr_devs, GFP_KERNEL);
    if (hc_devp){
        printk(KERN_WARNING "alloc mem failed");
        ret = -ENOMEM;
        goto failure_kzalloc;
    }
    for (i = 0; i < hello_nr_devs; i++){
        cdev_init(&hc_devp[i].cdev, &hc_flops);  /* 初始化字符设备结构*/
        hc_devp[i].cdev.owner = THIS_MODULE;
        /* 添加字符设备到系统 */
        ret = cdev_add(&hc_devp[i].cdev, MKDEV(hello_major, hello_minor + i), 1); 
        if (ret){
            printk(KERN_WARNING "fail add hc_dev%d", i); 
        }
    }
    printk(KERN_INFO "--END HELLO LINUX MODULE---\n");
    return 0;
fail_kzalloc:
    unregister_chrdev_region(devt, hello_nr_devs);
fail:
    return ret;
}

static void __exit hello_exit(void){
    int i;

    for (int i = 0; i < hello_nr_devs; i++){
        cdev_del(&hc_devp[i].cdev);
    }
    kfree(hc_devp);
    unregister_chrdev_region(devt, hello_nr_devs);
    printk(KERN_INFO "GOODBYE LINUX\n");
}

/* 宏 */
module_init(hello_init);
module_exit(hello_exit);

/* 描述性定义 */
MODULE_LICENSE("Dual BSD/GPL"); /* 许可的协议 */
MODULE_AUTHOR("XZX");  /* 作者 */
module_version("v1.0");  /* 版本 */


