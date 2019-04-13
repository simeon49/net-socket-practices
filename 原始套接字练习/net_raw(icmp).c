/*
*	网络层原始套接字
*	ICMP 请求 
*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/time.h>
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

int g_request_time = 0;
int g_respond_time = 0;

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
	struct icmp *p_icmp = (struct icmp *)(buff);
	struct sockaddr_in target;
	struct hostent *host;
	struct timeval tm;
	int cur_time;
	const int off = 0;
	int res;

	if(setsockopt(skfd, IPPROTO_IP, IP_HDRINCL, &off, sizeof(off)) < 0) {
		perror("IP_HDRINCL failed");
		exit(1);
	}

	gettimeofday(&tm, NULL);
	cur_time = tm.tv_sec * 1000 + tm.tv_usec / 1000; //(ms)

	//ICMP层
	p_icmp->icmp_type = ICMP_TSTAMP;
	p_icmp->icmp_code = 0;
	p_icmp->icmp_cksum = 0;
	p_icmp->icmp_id = htons(0x1234);
	p_icmp->icmp_seq = 0;
	p_icmp->icmp_otime = cur_time;
	p_icmp->icmp_rtime = 0;
	p_icmp->icmp_ttime = 0;

	bzero(&target, sizeof(struct sockaddr_in));
	target.sin_family = AF_INET;
	target.sin_port = 0;

	if (inet_aton(to_ip, &target.sin_addr) == 0) {
		host = gethostbyname(to_ip);
		if (host == NULL) {
			printf("TargetName Error:%s\n", hstrerror(h_errno));
			exit(1);
		}
		target.sin_addr = *(struct in_addr *)(host->h_addr_list[0]);
	}
	

	p_icmp->icmp_cksum = 0;
	p_icmp->icmp_cksum = check_sum((u_short *)p_icmp, sizeof(struct icmp));

	gettimeofday(&tm, NULL);
	g_request_time = tm.tv_sec * 1000 + tm.tv_usec / 1000; //(ms)
	res = sendto(skfd, buff, sizeof(struct icmp), 
			0, (struct sockaddr*)&target, sizeof(target));
	if (res < 0) {
		perror("sendto(): ");
	}
}

void send_icmp_hdrincl(int skfd, const char *to_ip)
{
	u_char buff[128] = {0};
	struct ip *p_ip = (struct ip *)(buff);
	struct icmp *p_icmp = (struct icmp *)(p_ip + 1);
	struct sockaddr_in target;
	struct hostent *host;
	const int on = 1;
	int res;
	struct timeval tm;
	uint32_t cur_time;

	bzero(&target, sizeof(struct sockaddr_in));
	target.sin_family = AF_INET;
	target.sin_port = 0;

	if (inet_aton(to_ip, &target.sin_addr) == 0) {
		host = gethostbyname(to_ip);
		if (host == NULL) {
			printf("TargetName Error:%s\n", hstrerror(h_errno));
			exit(1);
		}
		target.sin_addr = *(struct in_addr *)(host->h_addr_list[0]);
	}
	

	//用模板代码来开启IP_HDRINCL特性，我们完全自己手动构造IP报文
	if(setsockopt(skfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("IP_HDRINCL failed");
		exit(1);
	}
	p_ip->ip_v = IPVERSION;
	p_ip->ip_hl = sizeof(struct ip) >> 2;
	p_ip->ip_tos = 0;
	p_ip->ip_len = sizeof(struct ip) + sizeof(struct icmp);
	p_ip->ip_id = 0;
	p_ip->ip_off = 0;
	p_ip->ip_ttl = 64;
	p_ip->ip_p = IPPROTO_ICMP;
	p_ip->ip_sum = 0;
	p_ip->ip_src.s_addr = INADDR_ANY;
	p_ip->ip_dst = target.sin_addr;

	gettimeofday(&tm, NULL);
	cur_time = tm.tv_sec * 1000 + tm.tv_usec / 1000; //(ms)

	//ICMP层
	p_icmp->icmp_type = ICMP_TSTAMP;
	p_icmp->icmp_code = 0;
	p_icmp->icmp_cksum = 0;
	p_icmp->icmp_id = htons(0x1234);
	p_icmp->icmp_seq = 0;
	p_icmp->icmp_otime = cur_time;
	p_icmp->icmp_rtime = 0;
	p_icmp->icmp_ttime = 0;

	p_icmp->icmp_cksum = 0;
	p_icmp->icmp_cksum = check_sum((u_short *)p_icmp, sizeof(struct icmp));

	gettimeofday(&tm, NULL);
	g_request_time = tm.tv_sec * 1000 + tm.tv_usec / 1000; //(ms)
	res = sendto(skfd, buff, sizeof(struct ip) + sizeof(struct icmp), 
			0, (struct sockaddr*)&target, sizeof(target));
	if (res < 0) {
		perror("sendto(): ");
	}
}

void recv_icmp(int skfd)
{
	int efd, i, n, res;
	u_char buff[128] = {0};
	struct ip *p_ip = (struct ip *)(buff);
	struct icmp *p_icmp = (struct icmp *)(p_ip + 1);
	struct epoll_event event, *events;
	struct sockaddr_in from_addr;
	socklen_t addr_len = sizeof(from_addr);
	struct timeval tm;
	uint32_t cur_time, timeout_time;

	efd = epoll_create1(0);
	if (efd < 0) {
		printf("epoll_create1(): %s\n", strerror(errno));
		goto bad;
	}
	event.data.fd = skfd;
	event.events = EPOLLIN;
	res = epoll_ctl(efd, EPOLL_CTL_ADD, skfd, &event);
	if (res < 0) {
		printf("epoll_ctl(): %s\n", strerror(errno));
		goto bad;
	}

	events = (struct epoll_event *)calloc(1, sizeof(struct epoll_event));
	if (events == NULL) {
		printf("calloc(): %s\n", strerror(errno));
		goto bad;
	}

	gettimeofday(&tm, NULL);
	cur_time = tm.tv_sec * 1000 + tm.tv_usec / 1000; //(ms)
	timeout_time = cur_time + 3000;
	while (1) {
		gettimeofday(&tm, NULL);
		cur_time = tm.tv_sec * 1000 + tm.tv_usec / 1000; //(ms)
		if (cur_time >= timeout_time) {
			printf("time out!\n");
			goto bad;
		}
		n = epoll_wait(efd, events, 1, timeout_time - cur_time);	//3s
		if (n == 0) {
			printf("time out!\n");
			goto bad;
		}
		for (i = 0; i < n; i ++) {
			if ((events[i].events & EPOLLERR) ||  
				(events[i].events & EPOLLHUP) ||  
				(!(events[i].events & EPOLLIN))) {

				printf(".......... Err in Nat check ......\n");
				epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &event); 
				free(events);
				goto bad;
				
			} else if (skfd == events[i].data.fd) {
				res = recvfrom(skfd, buff, sizeof(buff), MSG_DONTWAIT, 
						(struct sockaddr *)&from_addr, &addr_len);
				if (res < 0) {
					goto bad;
				}
				printf("recv size=%d from: %s:%d ", 
						res, inet_ntoa(from_addr.sin_addr), ntohs(from_addr.sin_port));
				if (p_ip->ip_p != IPPROTO_ICMP)
					continue;
				switch (p_icmp->icmp_type) {
				case ICMP_ECHOREPLY:
					printf("Echo Reply\n");
					break;
				case ICMP_DEST_UNREACH:
					printf("Destination Unreachable\n");
					break;
				case ICMP_SOURCE_QUENCH:
					printf("Source Quench\n");
					break;
				case ICMP_REDIRECT:
					printf("Redirect (change route)\n");
					break;
				case ICMP_ECHO:
					printf("Echo Request\n");
					break;
				case ICMP_TIME_EXCEEDED:
					printf("Time Exceeded\n");
					break;
				case ICMP_PARAMETERPROB:
					printf("Parameter Problem\n");
					break;
				case ICMP_TSTAMP:
					printf("Timestamp Request\n");
					break;
				case ICMP_TIMESTAMPREPLY:
				{
					gettimeofday(&tm, NULL);
					g_respond_time = tm.tv_sec * 1000 + tm.tv_usec / 1000; //(ms)

					int rtime = ntohl(p_icmp->icmp_rtime);
					int hour = rtime / 3600 / 1000 + 8;
					int minute = rtime / 60 / 1000 % 60;
					float sec = rtime % 60000 / 1000.0;
					printf("Timestamp Reply %02d:%02d:%05.3f\n", hour, minute, sec);
					return;
					break;
				}
				case ICMP_INFO_REQUEST:
					printf("Information Request\n");
					break;
				case ICMP_INFO_REPLY:
					printf("Information Reply\n");
					break;
				case ICMP_ADDRESS:
					printf("Address Mask Request\n");
					break;
				case ICMP_ADDRESSREPLY:
					printf("Address Mask Reply\n");
					break;
				default:
					break;
				}
			}
		}
	}
	
 bad:
 	return;
}


int main(int argc, char** argv)
{
	int skfd;
	int rtt;

	if (argc != 2) {
		printf("Usage:%s target\n", argv[0]);
		exit(1);
	}
	if((skfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		perror("Create Error");
		exit(1);
	}

	//用户封装icmp ip头由内核生成
	send_icmp(skfd, argv[1]);
	recv_icmp(skfd);
	rtt = (g_respond_time - g_request_time) >> 1;
	if (rtt > 0)
		printf("RTT = %d\n", rtt);

	//sleep(3);

	//用户封装icmp 使用模板封装ip
	send_icmp_hdrincl(skfd, argv[1]);
	recv_icmp(skfd);
	rtt = (g_respond_time - g_request_time) >> 1;
	if (rtt > 0)
		printf("RTT = %d\n", rtt);

	close(skfd);
	return 0;
}

