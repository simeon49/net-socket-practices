/* ************************************************************************
 *       Filename:  socket_server.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2013年02月14日
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "socket_headfile.h"
//========================================================================
//	语法格式：	void * client_respond(void * arg)
//	实现功能：	响应客户端 接收客户发送的一个字符 并将该字符发回给客户
//	参数：		arg 响应客户端的套结字
//	返回值：	无
//========================================================================
void * client_respond(void * arg){
	char	ch;
	int	res;
	int	client_respond_sockfd = (int)arg;
	if((res = pthread_detach(pthread_self())) != 0){
		switch(res){
			case EINVAL:
				printf("thread is not a joinable thread.\n");
				break;
			case ESRCH:
				printf("No thread with the ID thread could be found.\n");
				break;
			default:
				printf("pthread_detach other error.\n");
		}
		exit(-1);
	}
	while(1){
		if(read(client_respond_sockfd, &ch, 1) == 0){		//注意 此处应该判断接收到的数据个数 在客户端先关闭时 read不会阻塞
			break;
		}
		printf(" ->%x\n", (unsigned char)ch);
		if(write(client_respond_sockfd, &ch, 1) != 1){
			break;
		}
	}
	close(client_respond_sockfd);
	printf("kill pthread %u.\n", (unsigned int)pthread_self());
	pthread_exit(NULL);
}

//========================================================================
//	语法格式：	void inet_socket(void)
//	实现功能：	建立套接字 绑定套接字（端口 ip 网络协议） 监听 创建响应客户
//				端的处理线程
//	参数：		int argc, char *argv[] 
//	返回值：	0
//========================================================================
void inet_socket(void){
	int	res;
	pthread_t	client_pthread;
	int	server_sockfd, client_sockfd;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	socklen_t	len;
	len = sizeof(client_address);
	if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		errexit("socket");
	}
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(SEVERIP);
	server_address.sin_port = htons(SEVERPORT);
	if(bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1){
		errexit("bind");
	}
	if(listen(server_sockfd, 7) == -1){
		errexit("listen");
	}
	printf("Server Waiting ...\n");
	while(1){
		if((client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &len)) == -1){
			errexit("accept");
		}
		printf("New pthread %u create for a client.\n", (unsigned int)client_pthread);
		if((res = pthread_create(&client_pthread, NULL, client_respond, (void *)client_sockfd)) != 0){
			switch(res){
				case EAGAIN:
					printf("Insufficient  resources  to  create  another thread, or \
							a system-imposed limit on the number of threads was encountered.\n");
					break;
				case EINVAL:
					printf("Invalid settings in attr.\n");
					break;
				case EPERM:
					printf("No permission to set the scheduling policy and parameters specified in attr.\n");
					break;
				default:
					printf("pthread_create other error.\n");
				}
			exit(-1);
		}
	}
	close(server_sockfd);
}


int main(int argc, char *argv[])
{
	inet_socket();
	return 0;
}

