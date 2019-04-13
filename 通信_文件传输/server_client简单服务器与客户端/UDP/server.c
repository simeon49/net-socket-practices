/* ************************************************************************
 *       Filename:  server.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月15日 10时04分03秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
unsigned short		port = 80;
int start_UDP_server(void)
{
	int					server_sockfd;
	struct sockaddr_in	server_addr;
	printf(" <start_UDP_server>Run Server(%d)...\n", port);
	if((server_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror(" <start_UDP_server>socket");
		_exit(-1);
	}
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror(" <start_UDP_server>bind");
		_exit(-1);
	}
	while(1)
	{
		int					len;
		struct sockaddr_in	client_addr;
		socklen_t			client_addr_len =  sizeof(client_addr);
		char				client_ip[INET_ADDRSTRLEN];
		char				buf[1024];
		memset(buf, '\0', sizeof(buf));
		if((len = recvfrom(server_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, &client_addr_len)) == -1)
		{
			printf(" <start_UDP_server>error:recvfrom\n");
			return -1;
		}
		inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, sizeof(client_ip));
		printf(" \e[34m(From :\e[36m%s %d)>> \e[32m%s\e[0m\n", client_ip, ntohs(client_addr.sin_port), buf);
		sendto(server_sockfd, buf, strlen(buf), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
	}
	printf("<start_UDP_server>  Server had stoped\n");
	close(server_sockfd);
	return 0;
}
int main(int argc, char *argv[])
{
	if(argc > 1)
	{
		port = atoi(argv[1]);
	}
	start_UDP_server();
	return 0;
}


