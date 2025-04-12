#include <linux/module.h>           // 内核模块机制
#include <linux/printk.h>           // 内核日志输出
#include <linux/netdevice.h>        // 网络设备核心结构体
#include <linux/etherdevice.h>      // 以太网设备初始化
#include <linux/skbuff.h>           // 网络数据包（sk_buff）
#include <linux/ip.h>               // IP头定义
#include <linux/in.h>               // 网络地址定义
#include <linux/if_arp.h>           // ARP协议相关定义
#include <linux/icmp.h>             // ICMP头定义

// 自定义交换MAC地址的函数
static void my_swapether(u8 *src, u8 *dst) {
    int i;
    u8 tmp;
    for ( i = 0; i < ETH_ALEN; i++) {
        tmp = src[i];
        src[i] = dst[i];
        dst[i] = tmp;
    }
}

static struct net_device *vnet_dev;

static void emulator_rx_packet(struct sk_buff *skb, struct net_device *dev) {
    struct ethhdr *eth = (struct ethhdr *)skb->data;        // 以太网头
    struct iphdr *ip = (struct iphdr *)(skb->data + ETH_HLEN); // IP头（ETH_HLEN=14字节）
    struct icmphdr *icmp = (struct icmphdr *)(ip + ip->ihl); // ICMP头（位于IP头之后）
    struct sk_buff *rx_skb;                                  // 应答数据包
    __be32 tmp_ip;                                           // 提前声明变量

    // 1. 交换MAC地址（源<->目的）
    my_swapether(eth->h_source, eth->h_dest);

    // 2. 交换IP地址（源<->目的）
    tmp_ip = ip->saddr;
    ip->saddr = ip->daddr;
    ip->daddr = tmp_ip;

    // 3. 构造ICMP应答（请求->应答）
    if (icmp->type == ICMP_ECHO) {                   // 仅处理ping请求
        icmp->type = ICMP_ECHOREPLY;                         // 设置为应答类型
        icmp->checksum = 0;                                  // 重置校验和
        icmp->checksum = ip_fast_csum((u8 *)icmp, sizeof(*icmp)); // 重新计算校验和
    }

    // 4. 分配应答数据包并复制数据
    rx_skb = dev_alloc_skb(skb->len);                        // 分配skb缓冲区
    if (!rx_skb) return;
    memcpy(rx_skb->data, skb->data, skb->len);               // 复制原始数据到应答包
    rx_skb->dev = dev;                                        // 关联网络设备
    rx_skb->protocol = eth->h_proto;                          // 设置协议类型（如0x0800=IP）
    rx_skb->ip_summed = CHECKSUM_UNNECESSARY;                // 虚拟包无需硬件校验

    // 5. 提交给协议层处理（模拟接收）
    dev->stats.rx_packets++;                                  // 更新接收统计
    dev->stats.rx_bytes += skb->len;
    netif_rx(rx_skb);                                         // 触发上层协议栈处理
}


static netdev_tx_t vnet_start_xmit(struct sk_buff *skb, struct net_device *dev) {
    printk(KERN_INFO "vnet: send packet, len=%d\n", skb->len); // 打印调试信息

    // 1. 暂停数据队列（防止并发发送）
    netif_stop_queue(dev);

    // 2. 模拟硬件应答（构造并提交接收包）
    emulator_rx_packet(skb, dev);                             // 核心虚拟逻辑：生成应答包

    // 3. 释放原始数据包（虚拟网卡无需真实发送）
    dev_kfree_skb(skb);

    // 4. 恢复数据队列（允许后续发送）
    netif_wake_queue(dev);

    // 5. 更新发送统计
    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;

    return NETDEV_TX_OK;                                      // 发送成功
}

// 网络设备操作函数集（仅实现必需的发送功能）
static const struct net_device_ops vnet_netdev_ops = {
    .ndo_start_xmit = vnet_start_xmit,                         // 数据发送入口
    //.ndo_set_mac_address = eth_mac_addr,                       // 设置MAC地址（使用以太网通用实现）
};


static int __init vnet_init(void) {
    // 1. 分配网络设备结构体（附带以太网设备初始化）
    vnet_dev = alloc_etherdev(0);                             // 0表示无私有数据，自动生成设备名（如vnet0）
    if (!vnet_dev) {
        printk(KERN_ERR "vnet: failed to allocate net device\n");
        return -ENOMEM;
    }

    // 2. 配置设备基本属性
    strncpy(vnet_dev->name, "vnet", IFNAMSIZ);                // 设置设备名称前缀
    vnet_dev->netdev_ops = &vnet_netdev_ops;                  // 绑定操作函数集
    vnet_dev->mtu = 1500;                                     // 设置MTU（默认以太网值）
    vnet_dev->flags = IFF_NOARP | IFF_UP | IFF_RUNNING;        // 设备标志：禁用ARP，已启动并运行

    // 3. 生成随机MAC地址（真实网卡需读取硬件，虚拟网卡可自定义）
    eth_hw_addr_random(vnet_dev);                             // 生成随机合法MAC地址

    // 4. 注册网络设备到内核
    if (register_netdev(vnet_dev)) {
        printk(KERN_ERR "vnet: failed to register net device\n");
        free_netdev(vnet_dev); // 使用free_netdev替代free_etherdev
        return -EIO;
    }

    printk(KERN_INFO "vnet: driver initialized, device %s\n", vnet_dev->name);
    return 0;
}


static void __exit vnet_exit(void) {
    unregister_netdev(vnet_dev);
    free_netdev(vnet_dev); // 使用free_netdev替代free_etherdev
    printk(KERN_INFO "vnet: driver unloaded\n");
}

// 模块声明
module_init(vnet_init);
module_exit(vnet_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Virtual Network Device Driver");
MODULE_AUTHOR("Your Name");