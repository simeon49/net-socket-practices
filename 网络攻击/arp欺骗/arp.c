
#include "arp.h"

static char err_buf[LIBNET_ERRBUF_SIZE];

#define ErrPrintG(_msg, _errstr) do{fprintf(stderr, "Error in function %s(): [%s] %s\n",  __func__, _msg, _errstr); goto bad;}while(0)

#define ErrPrintR(_msg, _errstr) do{fprintf(stderr, "Error in function %s(): [%s] %s\n",  __func__, _msg, _errstr); return -1;}while(0)

/*
 *	在ARP表中查找ip对应的mac ( inside )
 */
int arp_cache_lookup(const char *interface, in_addr_t ip, u_int8_t *ether)
{
	int sock;
	struct arpreq ar;
	struct sockaddr_in *sin;

	memset((char *)&ar, 0, sizeof(ar));
	strncpy(ar.arp_dev, interface, sizeof(ar.arp_dev));   /* XXX - *sigh* */
	sin = (struct sockaddr_in *)&ar.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ip;

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		ErrPrintR("socket", strerror(errno));

	//printf("Interface: %s   IP: %s\n", interface, inet_ntoa(*(struct in_addr *)&ip));
	if (ioctl(sock, SIOCGARP, (caddr_t)&ar) == -1) {
		close(sock);
		ErrPrintR("ioctl", strerror(errno));
	}
	close(sock);
	//printf("Get target ip's mac\n");
	memcpy(((struct ether_addr *)ether)->ether_addr_octet, ar.arp_ha.sa_data, ETHER_ADDR_LEN);

	return (0);
}

int arp_force(in_addr_t dst)
{
	struct sockaddr_in sin;
	int i, fd;

	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return (0);

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = dst;
	sin.sin_port = htons(67);

	i = sendto(fd, NULL, 0, 0, (struct sockaddr *)&sin, sizeof(sin));

	close(fd);

	return (i == 0);
}

/*
 *	发送arp包
 * 	interface: 网络介质 （例如：“wlan0” “eth0”）
 * 	op: arp选项 （参/libnet/libnet-headers.h）
 * 	sender_mac： arp包发送者MAC （NULL时为本机interface mac）
 * 	sender_ip： arp包发送者IP （0时为本机interface ip）
 * 	target_mac： arp包目的MAC （NULL时为广播）
 * 	target_ip： arp包目的IP
 * 	成功0  失败-1
 */
int arp_send(char *interface, int op, u_int8_t *sender_mac, in_addr_t sender_ip,  u_int8_t *target_mac, in_addr_t target_ip)
{
	libnet_t *l = NULL;

	if ((l = libnet_init(LIBNET_LINK_ADV, interface, err_buf)) == NULL)
		ErrPrintR("libnet_init", err_buf);

	if (sender_mac == NULL)
		if ((sender_mac = (u_int8_t *)libnet_get_hwaddr(l)) == NULL)
			ErrPrintG("libnet_get_hwaddr", libnet_geterror(l));

	if (sender_ip == 0)
		if ((sender_ip = libnet_get_ipaddr4(l)) == -1)
			ErrPrintG("sender_ip", libnet_geterror(l));

	if (target_mac == NULL)
		target_mac = (u_int8_t *)"\xff\xff\xff\xff\xff\xff";

	if (libnet_autobuild_arp(op, sender_mac, (u_int8_t *)&sender_ip, target_mac, (u_int8_t *)&target_ip, l) == (libnet_ptag_t)-1)
		ErrPrintG("libnet_autobuild_arp", libnet_geterror(l));

	if (libnet_autobuild_ethernet(target_mac, ETHERTYPE_ARP, l) == -1)
		ErrPrintG("libnet_autobuild_ethernet", libnet_geterror(l));

	if (libnet_write(l) == -1)
		ErrPrintG("libnet_write", libnet_geterror(l));
bad:
	libnet_destroy(l);
	return 0;
}


/*
 *	通过发送最多3次arp请求包 使arp表存在一个ip对应的mac项
 * 	interface: 网络介质 （例如：“wlan0” “eth0”）
 * 	ip：需要查找mac的IP
 * 	ether：用于保存mac地址（需要保证 最小6字节）
 *  成功 0  失败-1
 */
int arp_find(char *interface, in_addr_t ip, u_int8_t *ether)
{
	int i = 0;
	while (i < 3) {
		if (arp_cache_lookup(interface, ip, ether) == 0)
			return 0;
		arp_force(ip);
		arp_send(interface, ARPOP_REQUEST, NULL, 0, NULL, ip);
		i ++;
		sleep(1);
	}
	return -1;
}
