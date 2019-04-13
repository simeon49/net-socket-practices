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
 *	����arp��
 * 	interface: ������� �����磺��wlan0�� ��eth0����
 * 	op: arpѡ�� ����/libnet/libnet-headers.h��
 * 	sender_mac�� arp��������MAC ��NULLʱΪ����interface mac��
 * 	sender_ip�� arp��������IP ��0ʱΪ����interface ip��
 * 	target_mac�� arp��Ŀ��MAC ��NULLʱΪ�㲥��
 * 	target_ip�� arp��Ŀ��IP
 * 	�ɹ�0  ʧ��-1
 */
int arp_send(char *interface, int op, u_int8_t *sender_mac, in_addr_t sender_ip, u_int8_t *target_mac, in_addr_t target_ip);


/*
 *	ͨ���������3��arp����� ʹarp�����һ��ip��Ӧ��mac��
 * 	interface: ������� �����磺��wlan0�� ��eth0����
 * 	ip����Ҫ����mac��IP
 * 	ether�����ڱ���mac��ַ����Ҫ��֤ ��С6�ֽڣ�
 *  �ɹ� 0  ʧ��-1
 */
int arp_find(char *interface, in_addr_t ip, u_int8_t *ether);
