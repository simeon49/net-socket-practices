/* ************************************************************************
 *       Filename:  clien.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月13日 21时58分01秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/


#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
unsigned short	port = 8000;
char			server_IPv4[16] = "192.168.220.88";
int main(int argc, char *argv[])
{
	int					sockfd;
	struct sockaddr_in	sockaddr;
	unsigned int		IP;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror(" <main>socket");
		_exit(-1);
	}
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	inet_pton(AF_INET, server_IPv4, &IP);
	sockaddr.sin_addr.s_addr = IP;
	if(connect(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
	{
		close(sockfd);
		perror(" <main>connect");
		_exit(-1);
	}
	char		buf[1024] = " This is a test!";
	write(sockfd, buf, sizeof(buf));
	memset(buf, '\0', sizeof(buf));
	read(sockfd, buf, sizeof(buf));
	printf(" >%s\n",buf);
	close(sockfd);
	return 0;
}


