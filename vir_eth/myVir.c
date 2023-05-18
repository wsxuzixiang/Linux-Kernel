#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/etherdevice.h>

static struct net_device *vir_dev;

static void vir_dev_rcpacket(struct sk_buff *skb, struct net_device *dev)
{
    struct ethhdr *ethhdr;
    struct iphdr *ih;
    unsigned char *type;
    unsigned char tmp_dev_addr[ETH_ALEN];
    __be32 *saddr, *daddr, tmp;
    struct sk_buff *rx_skb;
    
    /* 1、对调 源/目 MAC地址 */
    ethhdr = (struct ethhdr *)skb->data;
    memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);
    memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);
    memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);

    /*  2、对调 源/目 IP地址 */
    ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
    saddr = &ih->saddr;
    daddr = &ih->daddr;
    tmp = *saddr;
    *saddr = *daddr;
    *daddr = tmp;

    /* 3、调用ip_fast_csum()获取iphr结构体的校验码 */
    ih->check = 0;
    ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);

    /* 4、设置数据类型为0，表示接收ping包 */
    type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
    *type = 0;

    /* 5、调用dev_alloc_skb()构造新的接收sk_buff */
    rx_skb = dev_alloc_skb(skb->len + 2);

    /*
    *   skb_reserve()增加头部空间
    *   skb_put()增加数据区长度
    */

    /* 6、调用skb_reserve()预留2字节的头部空间，16字节对齐，（以太网的协议头长度是14个字节） */
    skb_reserve(rx_skb, 2);

    /* 7、将skb_buff->data复制到新的sk_buff中 利用skb_put动态扩大数据区，避免溢出*/
    memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

    /* 8、设置新的sk_buff的其他成员 */
    rx_skb->dev = dev;
    rx_skb->ip_summed = CHECKSUM_UNNECESSARY;

    /* 9、调用eth_type_trans()获取上层协议 */
    rx_skb->protocol = eth_type_trans(rx_skb, dev);

    /* 10、更新接收的统计信息，调用netif_rx()来传递sk_buff数据包 */
    dev->stats.rx_packets ++;
    dev->stats.rx_bytes += skb->len;
    dev->last_rx = jiffies;

    netif_rx(rx_skb);

}

static int vir_dev_xmit(struct sk_buff *skb, struct net_device *dev)
{
    printk("Running vir_dev_xmit\n");

    /* 调用netif_stop_queue()阻止上层向网络设备驱动层发送数据包 */
    netif_stop_queue(dev);

    /* 调用收包函数，伪造接收ping包 */
    vir_dev_rcpacket(skb, dev);

    /* 调用dev_kfree_skb()函数释放发送的sk_buff */
    dev_kfree_skb(skb);

    /* 更新发送的统计信息 */
    dev->stats.tx_packets ++;
    dev->stats.tx_bytes += skb->len;
    dev->trans_start = jiffies;

    /* 调用netif_wake_queue()唤醒被阻塞的上层 */
    netif_wake_queue(dev);

    return 0;
}

static int vir_dev_open(struct net_device *dev)
{
    printk("Running vir_dev_open\n");
    /* 启动接口传输队列 */
    netif_start_queue(dev);
    return 0;
}

static int vir_dev_stop(struct net_device *dev)
{
    printk("Running vir_dev_stop\n");
    /* 停止接口传输队列 */
    netif_stop_queue(dev);
    return 0;
}

static const struct net_device_ops vir_dev_ops = {
    .ndo_open       = vir_dev_open,
    .ndo_stop       = vir_dev_stop,
    .ndo_start_xmit = vir_dev_xmit,
};

static int vir_dev_register(void)
{
    int ret = 0;

    vir_dev = alloc_netdev(sizeof(struct net_device), "eth_xzx", NET_NAME_UNKNOWN, ether_setup);
    if (IS_ERR(vir_dev)) {
        return -ENOMEM;
    }

    /* 初始化MAC地址 */
    vir_dev->dev_addr[0] = 0x00;
    vir_dev->dev_addr[1] = 0x01;
    vir_dev->dev_addr[2] = 0x02;
    vir_dev->dev_addr[3] = 0x03;
    vir_dev->dev_addr[4] = 0x04;
    vir_dev->dev_addr[5] = 0x05;

    /* 设置操作函数 */
    vir_dev->netdev_ops = &vir_dev_ops;
    vir_dev->flags      |= IFF_NOARP;
    vir_dev->features   |= NETIF_F_HW_CSUM;

    /* 注册net_device结构体 */
    ret = register_netdev(vir_dev);
    if (ret) {
        free_netdev(vir_dev);
        return ret;
    }

    return ret;
}

static void vir_dev_unregister(void)
{
    unregister_netdev(vir_dev);
    free_netdev(vir_dev);
}

static int __init vir_init(void)
{
    vir_dev_register();

    return 0;
}

static void __exit vir_exit(void)
{
    vir_dev_unregister();
}

module_init(vir_init);
module_exit(vir_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xzx");