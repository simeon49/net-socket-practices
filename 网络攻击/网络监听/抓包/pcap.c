/*	pcap	Packet capture library usage	*/
/*	www.tcpdump.org/pcap.htm	*/

#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <net/ethernet.h>	//Á´Â·²ã Í·
#include <linux/ip.h>
#include <linux/tcp.h>
#include <pcap.h>

//#define BUFSIZ 65535

void show_http_info(struct iphdr *ip_h, struct tcphdr *tcp_h, u_int8_t *payload, int payloads, int total_sizes)
{
	printf("\n\n\nHttp request from IP: %s", inet_ntoa(*(struct in_addr *)&(ip_h->saddr)));
	printf("  to IP: %s\n", inet_ntoa(*(struct in_addr *)&(ip_h->daddr)));

	int i = 0;
	while (i < payloads) {
		if (i %16 == 0)
			printf("\n");
		else if (i % 8 == 0)
			printf("    ");
		printf("%02x ", (unsigned char)*(payload+i));
		i ++;
	}
	printf("\nTotal Size=%d  Data Size=%d\n", total_sizes, payloads);
}

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	struct ether_header *ether_h;
	struct iphdr *ip_h;
	struct tcphdr *tcp_h;
	u_int8_t *tcp_opt_h;	//tcp_选项
	u_int8_t *payload;	//指向tcp包的有效负载
	int payloads;		//有效负载大小
	int tcp_opt_sizes;		//tcp_选项 长度

	/*	ethernet	*/
	ether_h = (struct ether_header *)packet;
	/*	IP	*/
	ip_h = (struct iphdr *)((char *)ether_h + sizeof(struct ether_header));
	/*	tcp	*/
	tcp_h = (struct tcphdr*)((char *)ip_h + ip_h->ihl * 4);
	tcp_opt_h = (u_int8_t *)((char *)tcp_h + sizeof(struct tcphdr));
	tcp_opt_sizes = tcp_h->doff * 4 - sizeof(struct tcphdr);
	payload = (u_int8_t *)((char *)tcp_h + tcp_h->doff * 4);
	payloads = header->len - ((int)payload - (int)packet);
	payload = (payloads? payload : NULL);

	show_http_info(ip_h, tcp_h, payload, payloads, header->len);
	int hour = ((header->ts.tv_sec) / 3600);
	int min = (header->ts.tv_sec - (3600 * hour)) / 60;
	float sec = (header->ts.tv_sec) % 60 + header->ts.tv_usec/1000000.0;
	printf("时间: %d(h)%d(m)%5.2f\n", hour % 24, min, sec);
	printf("帧长:%d  实际抓取到的长度:%d\n", header->caplen, header->len);
}

int main(int argc, char *argv[])
{
	pcap_t *handle;			/* Session handle */
	char *dev;			/* The device to sniff on */
	char errbuf[PCAP_ERRBUF_SIZE];	/* Error string */
	struct bpf_program fp;		/* The compiled filter */
	char filter_exp[] = "port 80";	/* The filter expression */
	bpf_u_int32 mask;		/* Our netmask */
	bpf_u_int32 net;		/* Our IP */
	struct pcap_pkthdr header;	/* The header that pcap gives us */
	const u_char *packet;		/* The actual packet */

	/* Define the device *//*
	dev = pcap_lookupdev(errbuf);
	if (dev == NULL) {
		fprintf(stderr, "Couldn't find default device: %s\n", errbuf);
		return(2);
	}*/
	dev = "wlan0";

	/* Find the properties for the device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n", dev, errbuf);
		net = 0;
		mask = 0;
	}
	/* Open the session in promiscuous mode */
	handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
	if (handle == NULL) {
		fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
		return(2);
	}
	/* Compile and apply the filter */
	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
	}
	/* Grab a packet */
	//packet = pcap_next(handle, &header);
	/* Print its length */
	//printf("Jacked a packet with length of [%d]\n", header.len);

	pcap_loop(handle, 5, got_packet, NULL);



	/* And close the session */
	pcap_close(handle);
	return(0);
}
