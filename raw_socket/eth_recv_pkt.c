#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <linux/if_ether.h>

#define MAX_PKT_LEN 2048

void print_hex(const unsigned char *data, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    int fd;
    int recv_pkt_num = 0;
    struct sockaddr_ll sll;
    struct ifreq ifr;
    unsigned char *pkt;
    int pkt_len;
    unsigned short target_port = 8080;

    if (argc != 3)
    {
        printf("Usage: %s <interface_name> <target_port>\n", argv[0]);
        exit(1);
    }
    target_port = atoi(argv[2]);

    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0)
    {
        perror("socket");
        exit(1);
    }

    strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("ioctl");
        exit(1);
    }

    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifr.ifr_ifindex;

    if (bind(fd, (struct sockaddr *)&sll, sizeof(sll)) < 0)
    {
        perror("bind");
        exit(1);
    }

    pkt = (unsigned char *)malloc(MAX_PKT_LEN);
    if (!pkt)
    {
        perror("malloc");
        exit(1);
    }

    while (1)
    {
        pkt_len = recv(fd, pkt, MAX_PKT_LEN, 0);
        if (pkt_len < 0)
        {
            perror("recv");
            continue;
        }

        // 以太网头部长度
        const unsigned char *eth_header = pkt;
        unsigned short eth_type = ntohs(((struct ethhdr *)eth_header)->h_proto);

        // 如果是 IP 数据包
        if (eth_type == ETH_P_IP)
        {
            const unsigned char *ip_header = pkt + sizeof(struct ethhdr);
            unsigned char ip_proto = ((struct iphdr *)ip_header)->protocol;

            // 如果是 UDP 数据包
            if (ip_proto == IPPROTO_UDP)
            {
                const unsigned char *udp_header = ip_header + ((struct iphdr *)ip_header)->ihl * 4;
                unsigned short dst_port = ntohs(((struct udphdr *)udp_header)->dest);

                // 如果目的端口匹配并且网卡匹配，打印数据包
                if (dst_port == target_port)
                {
                    printf("Received packet number %d,pkt_len %d\n", recv_pkt_num, pkt_len);
                    recv_pkt_num++;
                    print_hex(pkt, pkt_len);
                }
            }
        }
    }

    free(pkt);
    close(fd);
    return 0;
}