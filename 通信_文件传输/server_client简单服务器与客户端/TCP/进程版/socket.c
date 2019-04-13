/* ************************************************************************
 *       Filename:  socket.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月13日 16时34分29秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/
//服务器 迭代方式

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
unsigned short	port =8000;
int main(int argc, char *argv[])
{
	int					listen_socketfd;
	struct sockaddr_in	sever_addr, clien_addr;
	listen_socketfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&sever_addr, sizeof(sever_addr));
	sever_addr.sin_family = AF_INET;
	sever_addr.sin_port = htons(port);
	sever_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	char server_ip[16];
	inet_ntop(AF_INET, &sever_addr.sin_addr.s_addr,server_ip, INET_ADDRSTRLEN);
	printf(">>>%s\n",server_ip);
	bind(listen_socketfd, (struct sockaddr *)&sever_addr, sizeof(sever_addr));
	listen(listen_socketfd, 10);
	char	buf[1024];
	int		conn_socketfd;
	char	ip[16];
	socklen_t	addrlen = sizeof(clien_addr);
	bzero(buf, sizeof(buf));
	while(1)
	{
		conn_socketfd = accept(listen_socketfd, (struct sockaddr *)&clien_addr, &addrlen);
		inet_ntop(AF_INET, &clien_addr.sin_addr.s_addr, ip, INET_ADDRSTRLEN);
		printf("client ip : %s\n", ip);
		while(1)
		{
			if(read(conn_socketfd, buf, sizeof(buf)) <= 0)
				break;
			write(conn_socketfd, buf, strlen(buf));
		}
	}
	close(listen_socketfd);
	return 0;
}


