#ifndef _HELLO_CHR_LOCKED_H_
#define _HELLO_CHR_LOCKED_H_

/* 幻数，查看 /Documentation/userspace-api/ioctl/ioctl-number.rst */
#define HC_IOC_MAGIC 0x81

/* 清空分配的空间 */
#define HC_IOC_RESET       _IO(HC_IOC_MAGIC, 0)
#define HC_IOCP_GET_LENS   _IOR(HC_IOC_MAGIC, 1, int)
#define HC_IOCV_GET_LENS   _IO(HC_IOC_MAGIC, 2)
#define HC_IOCP_SET_LENS   _IOW(HC_IOC_MAGIC, 3, int)
#define HC_IOCV_SET_LENS   _IO(HC_IOC_MAGIC, 4)

#define HC_IOC_MAXNR 4

#endif 