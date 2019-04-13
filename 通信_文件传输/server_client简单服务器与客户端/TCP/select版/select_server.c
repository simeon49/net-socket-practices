/* ************************************************************************
 *       Filename:  select_server.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2013年02月16日
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include "socket_headfile.h"

void server(void){
	int	server_sockfd, client_sockfd;
	struct sockaddr_in server_address, client_address;
	int	server_len, client_len;
	fd_set	readfds, testfds;
	if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("socket\n");
		exit(0);
	}
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(SEVERPORT);
	server_len = sizeof(server_address);
	if(bind(server_sockfd, (struct sockaddr *)&server_address, server_len) == -1){
		perror("bind\n");
		exit(0);
	}
	if(listen(server_sockfd, 5) == -1){
		perror("listen\n");
		exit(0);
	}
	/*	使用select监听socket套接字	*/
	FD_ZERO(&readfds);
	FD_SET(server_sockfd, &readfds); 
	while(1){
		int	rs;
		int	fd;
		char ch;
		int nread;
		struct timeval	daly_time;
		daly_time.tv_sec = 3;
		daly_time.tv_usec = 0;
		/*	select函数有可能会改变被监听的文件描述符集 所以应该每次都要备份	*/
		testfds = readfds;
		rs = select(FD_SETSIZE, &testfds, (fd_set *)NULL, (fd_set *)NULL, &daly_time);
		if(rs == -1){
			perror("select");
			exit(0);
		}else if(rs == 0){	//时间到 
			printf("Server Waiting!\n");
		}else{
			/*	检测每一个描述符	*/
			for(fd = 0; fd < FD_SETSIZE; fd ++){
				if(FD_ISSET(fd, &testfds)){
					if(fd == server_sockfd){	//当server_sockfd可读时 表示有新的客户链接请求
						if((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, (socklen_t *)&client_len)) == -1){
							perror("accept\n");
							exit(0);
						}
						FD_SET(client_sockfd, &readfds);
						printf("New client whit port %d  fd:%d\n", ntohs(client_address.sin_port), client_sockfd);
					}else{
						ioctl(fd, FIONREAD, &nread);
						if(nread == 0){		//当没有数据可读时表示 客户端关闭了端口
							close(fd);
							FD_CLR(fd, &readfds);
							printf("removing a client fd:%d\n", fd);
						}else{
							if(read(fd, &ch, 1) < 0){
								perror("read");
								exit(0);
							}
							printf("read from fd:%d ->%x\n", fd, (unsigned char)ch);
							if(write(fd, &ch, 1) < 0){
								perror("write");
								exit(0);
							}
						}
					}
				}
			}
		}
	}
}
int main(int argc, char *argv[])
{
	server();
	return 0;
}


