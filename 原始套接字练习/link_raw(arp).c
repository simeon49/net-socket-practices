/*
*	链路层原始套接字
*	ARP 请求 
*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <linux/tcp.h>


void send_arp(int skfd, char *arp_request_ip)
{
	u_char buff[128] = {0};
	struct ether_header *p_eth = (struct ether_header *)buff;
	struct ether_arp *p_arp = (struct ether_arp *)(p_eth + 1);
	struct sockaddr_ll toaddr;
	struct in_addr target_addr, src_addr;
	unsigned char src_mac[ETH_ALEN], dst_mac[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct ifreq ifr;
	int res;

	bzero(&toaddr, sizeof(toaddr));
	bzero(&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, "wlan0");
	//获取接口索引
	if(-1 == ioctl(skfd, SIOCGIFINDEX, &ifr)){
		   perror("get dev index error:");
		   exit(1);
	}
	toaddr.sll_ifindex = ifr.ifr_ifindex;
	printf("interface Index:%d\n", ifr.ifr_ifindex);

	//获取接口的MAC地址
	if(-1 == ioctl(skfd, SIOCGIFHWADDR, &ifr)){
		   perror("get dev MAC addr error:");
		   exit(1);
	}
	memcpy(src_mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	printf("MAC :%02x-%02x-%02x-%02x-%02x-%02x\n",
			src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);

	//链路层
	memcpy(p_eth->ether_dhost, dst_mac, ETH_ALEN);
	memcpy(p_eth->ether_shost, src_mac, ETH_ALEN);
	p_eth->ether_type = htons(ETHERTYPE_ARP);
	
	//手动开始填充用ARP报文首部
	p_arp->arp_hrd = htons(ARPHRD_ETHER); //硬件类型为以太
	p_arp->arp_pro = htons(ETHERTYPE_IP); //协议类型为IP

	//硬件地址长度和IPV4地址长度分别是6字节和4字节
	p_arp->arp_hln = ETH_ALEN;
	p_arp->arp_pln = 4;

	//操作码，这里我们发送ARP请求
	p_arp->arp_op = htons(ARPOP_REQUEST);
	  
	//填充发送端的MAC和IP地址
	memcpy(p_arp->arp_sha, src_mac, ETH_ALEN);
	//获取接口IP地址
	if(-1 == ioctl(skfd, SIOCGIFADDR, &ifr)) {
		perror("get IP addr error:");
		exit(1);
	}
	src_addr.s_addr = ((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr.s_addr;
	printf("IP addr:%s\n",inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
	memcpy(p_arp->arp_spa, &src_addr, 4);

	//填充目的端的IP地址，MAC地址不用管
	inet_pton(AF_INET, arp_request_ip, &target_addr);
	memcpy(p_arp->arp_tpa, &target_addr, 4);

	toaddr.sll_family = PF_PACKET;
	res = sendto(skfd, buff, sizeof(struct ether_header) + sizeof(struct ether_arp), 
			0, (struct sockaddr*)&toaddr, sizeof(toaddr));
	if (res < 0) {
		perror("sendto(): ");
	}
}

int main(int argc,char** argv)
{
	int skfd;

	if (argc != 2) {
		printf("Usage:%s arp_request_ip \n", argv[0]);
		printf("	$ %s 192.168.1.1 \n", argv[0]);
		exit(1);
	}

	if((skfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		perror("Create Error");
		exit(1);
	}

	send_arp(skfd, argv[1]);

	close(skfd);
	return 0;
}

