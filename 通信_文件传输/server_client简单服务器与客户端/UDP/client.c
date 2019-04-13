/* ************************************************************************
 *       Filename:  client.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月15日 10时46分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int client(char ip[], int port, char *str)
{
	int					len;
	char				buf[1024];
	int					client_sockfd;
	struct sockaddr_in	client_sockaddr;
	struct sockaddr_in	from_sockaddr;
	socklen_t			from_sockaddr_len;
	printf(" <client>Run client\n");
	if((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror(" <client>socket");
		_exit(-1);
	}
	memset(&client_sockaddr, 0, sizeof(client_sockaddr));
	client_sockaddr.sin_family = AF_INET;
	client_sockaddr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &client_sockaddr.sin_addr.s_addr);
	if((len = sendto(client_sockfd, str, strlen(str), 0, (struct sockaddr *)&client_sockaddr, sizeof(client_sockaddr))) == -1)
	{
		printf(" <client>error:(To:%s:%d)\n", ip, port);
		return -1;
	}
	memset(buf, '\0', sizeof(buf));
	if((len = recvfrom(client_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from_sockaddr, &from_sockaddr_len)) == -1)
	{
		printf(" <clinet>error:recvfrom\n");
		return -1;
	}
	printf(" >>%s\n", buf);
	return 0;
}
int main(int argc, char *argv[])
{
	char		to_ip[INET_ADDRSTRLEN];
	int			to_port;
	char		msg[1024];
	printf("<Case>192.168.220.88  8000  \"This is a msg!\"\n");
	if(argc == 4)
	{
		strncpy(to_ip, argv[1], sizeof(to_ip));
		printf(" ->%s\n", to_ip);
		to_port = atoi(argv[2]);
		printf(" ->%d\n", to_port);
		strncpy(msg, argv[3], sizeof(msg));
		printf(" ->%s\n", msg);
		client(to_ip, to_port, msg);
	}
	else
	{
		printf(" <main>Not right!\n");
	}
	return 0;
}


