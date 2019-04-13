
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#include "get_IP.h"

#define ErrPrintR(_msg) do{fprintf(stderr, "[get_IP]error: %s %s\n", _msg, strerror(errno)); return -1;}while(0)

int get_ip(char *IP)
{
	struct sockaddr_in addr;
	struct hostent *host;
	int fd_tcp;
	int opt_value = 1;
	char buf[1024];

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(80);

	if ((host = gethostbyname("checkip.dyndns.org")) == NULL)
		ErrPrintR("gethostbyname");
	addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
	printf("[get_IP] Connet http://checkip.dyndns.org(%s) to get IP\n", inet_ntoa(addr.sin_addr));

	if ((fd_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		ErrPrintR("socket");

	if ((setsockopt(fd_tcp, SOL_SOCKET, SO_REUSEADDR, (void *)&opt_value, sizeof(opt_value))) == -1)
		ErrPrintR("setsockopt");

	if (connect(fd_tcp, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		ErrPrintR("connect");

	memset(buf, '\0', sizeof(buf));
	sprintf(buf, "GET / HTTP//1.1\r\nAccept: text/html\r\n\r\n");

	if (send(fd_tcp, buf, strlen(buf), 0) == -1)
		ErrPrintR("send");

	memset(buf, '\0', sizeof(buf));
	if (recv(fd_tcp, buf, sizeof(buf), 0) == -1)
		ErrPrintR("recv");
	//printf("->%s\n", buf);

	if (strncmp(buf, "HTTP/1.1 200", 12) == 0) {
		sscanf(strstr(buf, "Current IP Address:"), "Current IP Address: %[0-9.]", IP);
		printf("[get_IP] IP:%s(end)\n", IP);
		close(fd_tcp);
		return 0;
	}

	close(fd_tcp);
	return -1;
}
