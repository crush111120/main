// 操作步骤如下：
// 1. 修改smac，86-91行
// 2. 修改dmac，93-98行
// 修改完成后，在终端执行： gcc eth_pkt_send.c -o eth_pkt_send
// 然后在终端运行: sudo ./eth_pkt_send enp60s0 192.168.1.1 192.168.1.2 8888 8889 1

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h> /*memcpy*/
#include <strings.h>/*bzero*/
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>/*htons,ntohs*/
#include <endian.h>   /*htobeXX,beXXtoh,htoleXX,leXXtoh*/
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/in.h>      /*struct sockaddr_in*/
#include <linux/if_ether.h>/*struct ethhdr*/
#include <linux/ip.h>      /*struct iphdr*/
#include <sys/ioctl.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <sys/file.h>
#include <sys/time.h>
#include <linux/filter.h>

#define MAX_PKT_LEN 2048 /*最大报文长度*/

/*普通以太网报文头部*/
typedef struct
{
    unsigned char dmac[6];   // dmac
    unsigned char smac[6];   // 源mac
    unsigned short eth_type; /* 以太网类型 */
} __attribute__((packed)) eth_header;

/*普通IP报文头部*/
typedef struct
{
    unsigned char ver;        /* 版本和头长度 */
    unsigned char tos;        /* 服务类型 */
    unsigned short ip_len;    // IP报文的总长度
    unsigned short ip_id;     // IP报文标识
    unsigned short ip_off;    // IP报文偏移
    unsigned char ttl;        // TTL字段
    unsigned char protocol;   // IP协议
    unsigned short check_sum; // 校验和
    unsigned char src_ip[4];  // 源IP
    unsigned char dst_ip[4];  // 目的IP
} __attribute__((packed)) ip_header;

/*普通udp报文头部*/
typedef struct
{
    unsigned short src_port;  // 源端口
    unsigned short dst_port;  // 目的端口
    unsigned short udp_len;   // udp长度
    unsigned short check_sum; // 校验和
} __attribute__((packed)) udp_header;

/*普通以太网报文*/
typedef struct
{
    eth_header eth_header;              /* 以太网头 14字节*/
    ip_header ip_header;                /* ip头 20字节 */
    udp_header udp_header;              /* udp头 8字节 */
    unsigned char payload[MAX_PKT_LEN]; /* payload */
} __attribute__((packed)) pt_pkt;

// 解析IP地址字符串为4字节数组
int parse_ip_address(const char *ip_str, unsigned char *ip_array)
{
    unsigned int a, b, c, d;
    if (sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d) != 4)
    {
        return -1;
    }
    if (a > 255 || b > 255 || c > 255 || d > 255)
    {
        return -1;
    }
    ip_array[0] = (unsigned char)a;
    ip_array[1] = (unsigned char)b;
    ip_array[2] = (unsigned char)c;
    ip_array[3] = (unsigned char)d;
    return 0;
}

// IP校验和计算函数
unsigned short ip_checksum(unsigned short *ptr, int nbytes)
{
    long sum;
    unsigned short oddbyte;
    short answer;

    sum = 0;
    while (nbytes > 1)
    {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1)
    {
        oddbyte = 0;
        *((unsigned char *)&oddbyte) = *(unsigned char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short)~sum;

    return answer;
}

// 报文头固定字段赋值
void generate_pt_pkt_header(unsigned char *buf, unsigned char *src_ip, unsigned char *dst_ip,
                            unsigned short src_port, unsigned short dst_port, int payload_len)
{
    pt_pkt *pkt = NULL;

    pkt = (pt_pkt *)buf;

    // ========== 以太网头赋值 ==========
    // 源MAC地址 - 请修改为发送端网卡的实际MAC地址
    pkt->eth_header.smac[0] = 0x00; // 请修改为实际MAC
    pkt->eth_header.smac[1] = 0x1a; // 请修改
    pkt->eth_header.smac[2] = 0x2b; // 请修改
    pkt->eth_header.smac[3] = 0x3c; // 请修改
    pkt->eth_header.smac[4] = 0x4d; // 请修改
    pkt->eth_header.smac[5] = 0x5e; // 请修改

    // 目的MAC地址 - 请修改为接收端网卡的实际MAC地址
    pkt->eth_header.dmac[0] = 0x00; // 请修改为实际MAC
    pkt->eth_header.dmac[1] = 0x1a; // 请修改
    pkt->eth_header.dmac[2] = 0x2b; // 请修改
    pkt->eth_header.dmac[3] = 0x3c; // 请修改
    pkt->eth_header.dmac[4] = 0x4d; // 请修改
    pkt->eth_header.dmac[5] = 0x5f; // 请修改

    pkt->eth_header.eth_type = htons(0x0800); // IPv4

    // ========== IP头赋值 ==========
    pkt->ip_header.ver = 0x45; // IPv4, 头长度20字节
    pkt->ip_header.tos = 0;
    // IP总长度 = IP头(20) + UDP头(8) + payload长度
    pkt->ip_header.ip_len = htons(20 + 8 + payload_len);
    pkt->ip_header.ip_id = htons(1);
    pkt->ip_header.ip_off = htons(0);
    pkt->ip_header.ttl = 64;
    pkt->ip_header.protocol = 0x11; // UDP协议
    pkt->ip_header.check_sum = 0;

    // 设置源IP和目的IP
    memcpy(pkt->ip_header.src_ip, src_ip, 4);
    memcpy(pkt->ip_header.dst_ip, dst_ip, 4);

    // 计算IP校验和
    pkt->ip_header.check_sum = ip_checksum((unsigned short *)&pkt->ip_header, 20);

    // ========== UDP头赋值 ==========
    pkt->udp_header.src_port = htons(src_port);
    pkt->udp_header.dst_port = htons(dst_port);
    // UDP长度 = UDP头(8) + payload长度
    pkt->udp_header.udp_len = htons(8 + payload_len);
    pkt->udp_header.check_sum = 0; // UDP校验和设置为0

    return;
}

// 打印报文内容（调试用）
void cnc_pkt_print(unsigned char *pkt, int len)
{
    int i = 0;

    printf("-----------------------***CNC PACKET***-----------------------\n");
    printf("Packet length: %d bytes\n", len);

    for (i = 0; i < len; i++)
    {
        if (i % 16 == 0)
            printf("%04X: ", i);
        printf("%02X ", *((unsigned char *)pkt + i));
        if (i % 16 == 15)
            printf("\n");
    }
    if (len % 16 != 0)
        printf("\n");
    printf("-----------------------***CNC PACKET***-----------------------\n\n");
}

void print_usage(char *program_name)
{
    printf("Usage: %s <interface> <src_ip> <dst_ip> <src_port> <dst_port> <num>\n", program_name);
    printf("Example: %s enp60s0 192.168.1.10 192.168.1.20 8888 8889 1\n", program_name);
    // printf("\nNote: Before compiling, please modify the MAC addresses in generate_pt_pkt_header function.\n");
}

int main(int argc, char *argv[])
{
    int fd, n;
    int i = 0;
    int k = 0;

    unsigned char *pkt;
    pt_pkt *tmp_pkt;

    // 参数检查
    if (argc != 7)
    {
        print_usage(argv[0]);
        return -1;
    }

    // 解析IP地址
    unsigned char src_ip[4], dst_ip[4];
    if (parse_ip_address(argv[2], src_ip) != 0)
    {
        printf("Error: Invalid source IP address format\n");
        print_usage(argv[0]);
        return -1;
    }
    if (parse_ip_address(argv[3], dst_ip) != 0)
    {
        printf("Error: Invalid destination IP address format\n");
        print_usage(argv[0]);
        return -1;
    }

    // 解析端口号
    unsigned short src_port = (unsigned short)atoi(argv[4]);
    unsigned short dst_port = (unsigned short)atoi(argv[5]);
    unsigned short num = (unsigned short)atoi(argv[6]);

    if (src_port == 0 || dst_port == 0)
    {
        printf("Error: Invalid port number\n");
        print_usage(argv[0]);
        return -1;
    }

    if (num == 0)
    {
        printf("Error: Invalid packet count\n");
        print_usage(argv[0]);
        return -1;
    }

    // 设置payload长度（固定60字节，可以根据需要修改）
    int payload_len = 60;

    // 打印配置信息
    printf("========== Configuration ==========\n");
    printf("Interface: %s\n", argv[1]);
    printf("Source IP: %s\n", argv[2]);
    printf("Destination IP: %s\n", argv[3]);
    printf("Source Port: %d\n", src_port);
    printf("Destination Port: %d\n", dst_port);
    printf("Packet Count: %d\n", num);
    printf("Payload Length: %d bytes\n", payload_len);
    printf("===================================\n\n");

    // 计算总报文长度：以太网头(14) + IP头(20) + UDP头(8) + payload
    int total_pkt_len = 14 + 20 + 8 + payload_len;
    if (total_pkt_len > MAX_PKT_LEN)
    {
        printf("Error: Total packet length %d exceeds MAX_PKT_LEN %d\n", total_pkt_len, MAX_PKT_LEN);
        return -1;
    }

    // 分配内存
    pkt = (unsigned char *)malloc(total_pkt_len);
    if (NULL == pkt)
    {
        printf("malloc buf fail\n");
        return -1;
    }

    bzero(pkt, total_pkt_len);

    // 创建原始套接字
    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0)
    {
        perror("socket fail");
        exit(1);
    }

    // 指定网卡名称
    struct ifreq req;
    char net_interface[16] = {0};
    sprintf(net_interface, "%s", argv[1]);
    strncpy(req.ifr_name, net_interface, IFNAMSIZ);

    if (-1 == ioctl(fd, SIOCGIFINDEX, &req))
    {
        printf("ioctl error! Interface %s not found.\n", net_interface);
        close(fd);
        return -1;
    }

    // 将网络接口赋值给原始套接字地址结构
    struct sockaddr_ll sll;
    bzero(&sll, sizeof(sll));
    sll.sll_ifindex = req.ifr_ifindex;

    // 生成报文头
    generate_pt_pkt_header(pkt, src_ip, dst_ip, src_port, dst_port, payload_len);

    // 填充payload数据
    tmp_pkt = (pt_pkt *)pkt;
    for (k = 0; k < payload_len; k++)
    {
        tmp_pkt->payload[k] = k % 256; // 填充测试数据
    }

    // 打印报文内容（调试用）
    // printf("Packet header preview (first 64 bytes):\n");
    cnc_pkt_print((unsigned char *)tmp_pkt, total_pkt_len < 64 ? total_pkt_len : 64);

    // printf("Start sending %d packets...\n", num);

    // 发送报文
    while (i < num)
    {
        printf("Sending packet %d/%d\n", i + 1, num);

        n = sendto(fd, pkt, total_pkt_len, 0, (struct sockaddr *)&sll, sizeof(sll));
        if (n == -1)
        {
            perror("sendto failed");
            exit(1);
        }

        if (n != total_pkt_len)
        {
            printf("Warning: Sent %d bytes, expected %d bytes\n", n, total_pkt_len);
        }
        else
        {
            printf("Success: Sent %d bytes\n", n);
        }

        if (i < num - 1)
        {
            usleep(8000); // 8毫秒发送间隔
        }
        i++;
    }

    // printf("\nAll %d packets sent successfully!\n", num);

    // 清理
    free(pkt);
    close(fd);

    return 0;
}
