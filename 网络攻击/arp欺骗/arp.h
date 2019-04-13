#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libnet.h>
#include <pcap.h>


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
int arp_send(char *interface, int op, u_int8_t *sender_mac, in_addr_t sender_ip, u_int8_t *target_mac, in_addr_t target_ip);


/*
 *	通过发送最多3次arp请求包 使arp表存在一个ip对应的mac项
 * 	interface: 网络介质 （例如：“wlan0” “eth0”）
 * 	ip：需要查找mac的IP
 * 	ether：用于保存mac地址（需要保证 最小6字节）
 *  成功 0  失败-1
 */
int arp_find(char *interface, in_addr_t ip, u_int8_t *ether);
