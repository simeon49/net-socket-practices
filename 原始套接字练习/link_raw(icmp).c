/*
*	链路层原始套接字
*	ICMP 请求 
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

//CRC
u_short check_sum(u_short *buffer, int size)
{
	u_int cksum = 0;

	while (size > 1) {
		cksum += *buffer++;
		size -= sizeof(u_short);
	}
	if (size > 0) {
		cksum += *(u_char *)buffer;
	}

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (u_short)(~cksum);
}

void send_icmp(int skfd, const char *to_ip)
{
	u_char buff[128] = {0};
	struct ether_header *p_eth = (struct ether_header *)buff;
	struct ip *p_ip = (struct ip *)(p_eth + 1);
	struct icmp *p_icmp = (struct icmp *)(p_ip + 1);
	struct sockaddr_ll toaddr;
	unsigned char src_mac[ETH_ALEN], dst_mac[ETH_ALEN] = {0x00, 0x25, 0x86, 0x1c, 0x7a, 0xc0};
	struct ifreq ifr;
	int res, ip_len, n;

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
	p_eth->ether_type = htons(ETHERTYPE_IP);
	
	//IP层
	ip_len = sizeof(struct ip) + sizeof(struct icmp);
	p_ip->ip_v = IPVERSION;
	p_ip->ip_hl = sizeof(struct ip)>>2;
	p_ip->ip_tos = 0;
	p_ip->ip_len = htons(ip_len);
	p_ip->ip_id = htons(0x1234);
	p_ip->ip_off = 0;
	p_ip->ip_ttl = 64;
	p_ip->ip_p = IPPROTO_ICMP;
	p_ip->ip_sum = 0;
	//获取接口IP地址
	if(-1 == ioctl(skfd, SIOCGIFADDR, &ifr)){
		perror("get IP addr error:");
		exit(1);
	}
	p_ip->ip_src.s_addr = ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr;
	//inet_aton("192.168.2.123", &p_ip->ip_src);
	inet_aton(to_ip, &p_ip->ip_dst);

	//ICMP层
	p_icmp->icmp_type = ICMP_TSTAMP;
	p_icmp->icmp_code = 0;
	p_icmp->icmp_cksum = 0;
	p_icmp->icmp_id = htons(0x1234);
	p_icmp->icmp_seq = 0;
	p_icmp->icmp_otime = htonl(0x98900);
	p_icmp->icmp_rtime = 0;
	p_icmp->icmp_ttime = 0;
	
	n = 3;
	while (n --) {
		p_icmp->icmp_cksum = 0;
		p_icmp->icmp_cksum = check_sum((u_short *)p_icmp, sizeof(struct icmp));

		p_ip->ip_sum = 0;
		p_ip->ip_sum = check_sum((u_short *)p_ip, sizeof(struct ip) + sizeof(struct icmp));

		toaddr.sll_family = PF_PACKET;
		res = sendto(skfd, buff, sizeof(struct ether_header) + sizeof(struct ip) + sizeof(struct icmp), 
				0, (struct sockaddr*)&toaddr, sizeof(toaddr));
		if (res < 0) {
			perror("sendto(): ");
		}
	}
}

int main(int argc, char** argv)
{
	int skfd;
	// struct sockaddr_in target;
	// struct hostent *host;
	// const int on = 1;
	// const int off = 0;

	if (argc != 2) {
		printf("Usage:%s target\n", argv[0]);
		exit(1);
	}

	// bzero(&target, sizeof(struct sockaddr_in));
	// target.sin_family = AF_INET;
	// target.sin_port = 0;

	/*if (inet_aton(argv[1], &target.sin_addr) == 0) {
		host = gethostbyname(argv[1]);
		if (host == NULL) {
			printf("TargetName Error:%s\n", hstrerror(h_errno));
			exit(1);
		}
		target.sin_addr = *(struct in_addr *)(host->h_addr_list[0]);
	}*/

	if((skfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		perror("Create Error");
		exit(1);
	}

	//用模板代码来开启IP_HDRINCL特性，我们完全自己手动构造IP报文
	/*if(setsockopt(skfd, IPPROTO_IP, IP_HDRINCL, &off, sizeof(off)) < 0) {
		perror("IP_HDRINCL failed");
		exit(1);
	}*/

	//因为只有root用户才可以play with raw socket :)
	//setuid(getpid());
	send_icmp(skfd, argv[1]);

	close(skfd);
	return 0;
}

