/* ************************************************************************
 *       Filename:  easy_talk.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月15日 14时34分47秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define		CMD_QUIT	-1									//退出 
#define		CMD_SAND_TO	1									//指定接收者IP

int					sockfd;
struct	sockaddr_in	sockaddr;

//========================================================================
//	语法格式：	int AnalysStr(char *str)
//	实现功能：	分析用户输入 并处理
//	参数：		str：用户输入字符
//	返回值：	无
//========================================================================
int AnalysStr(char *str)
{
	char temp[1024];
	if(strncmp(str, "quit", 4) ==0 )
		return CMD_QUIT;
	else if(strncmp(str, "Talkto:" , 7) == 0)
	{
		strncpy(temp, str, sizeof(temp));
		memset(str, '\0', sizeof(str));
		strcpy(str, strrchr(temp, ':') + 1);
		*(char *)(strrchr(str, '\n')) = '\0';
		return CMD_SAND_TO;
	}
	else
		return 0;
}

//========================================================================
//	语法格式：	void * SendMsg(void * arg)
//	实现功能：	接收用户输入 并向指定IP（端口） 发送数据
//	参数：		无
//	返回值：	无
//========================================================================
void * SendMsg(void * arg)
{
	int					len;
	char				buf[1024];	
	unsigned int		port = (unsigned int)arg;			//端口
	struct sockaddr_in	send_sockaddr;
	memset(&send_sockaddr, 0, sizeof(send_sockaddr));
	send_sockaddr.sin_family = AF_INET;											//使用IPv4协议
	send_sockaddr.sin_port = htons(port);
	while( 1 )																	//输入"quit"退出程序
	{
		memset(buf, '\0', sizeof(buf));
		fgets(buf, sizeof(buf), stdin);
		switch(AnalysStr(buf))
		{
			case CMD_SAND_TO:													//指定IP
				printf(" >>|%s|%d\n", buf, strlen(buf));
				if(inet_pton(AF_INET, buf, &send_sockaddr.sin_addr) == -1)
				{
					printf(" <SandMsg>error: %s is not a IP\n",buf);
					continue;
				}
				printf(" >>>>>%d\n", send_sockaddr.sin_addr.s_addr);
				memset(buf, '\0', sizeof(buf));
				strcpy(buf, "[ring]\n");
				break;
			case CMD_QUIT:														//退出
				goto END;
				break;
			default:															
				break;
		}
		if((len = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&send_sockaddr, sizeof(send_sockaddr))) == -1)
		{																		//发送信息
			printf(" <SandMsg>error: sendto(msg)\n");
			close(sockfd);
			pthread_exit(NULL);
		}
	}
END:
	close(sockfd);
	return NULL;
}

//========================================================================
//	语法格式：	void * ReceiveMsg(void * arg)
//	实现功能：	从网络上接收信息
//	参数：		无
//	返回值：	无
//========================================================================
void * ReceiveMsg(void * arg)
{
	int					len;
	char				buf[1024];
	struct sockaddr_in	receive_sockaddr;
	socklen_t			receive_sockaddr_len = sizeof(receive_sockaddr);
	char				receive_IP[INET_ADDRSTRLEN];
	unsigned short		receive_port;
	while( 1 )
	{
		memset(buf, '\0', sizeof(buf));											//接收信息
		if((len = recvfrom(sockfd, buf, sizeof(buf), 0,(struct sockaddr *)&receive_sockaddr, &receive_sockaddr_len)) == -1)
		{
			printf(" <ReceiveMsg>error:recvfrom\n");
			printf(" <ReceiveMsg>error:recvfrom\n");
			close(sockfd);
			pthread_exit(NULL);
		}
		inet_ntop(AF_INET, &receive_sockaddr.sin_addr.s_addr, receive_IP, sizeof(receive_IP));
		receive_port = ntohs(receive_sockaddr.sin_port);
		printf(" (From %s :%d) %s\n", receive_IP, receive_port, buf);
	}
	close(sockfd);
	return NULL;
}
//========================================================================
//	语法格式：	int main(int argc, char *argv[])
//	实现功能：	创建socket 并绑定  创建接收 发送线程
//	参数：		argc argv ：需要时 用户指定端口（收 发）
//	返回值：	无
//========================================================================
int main(int argc, char *argv[])
{
	pthread_t			receive_pthread;					//接收消息线程
	pthread_t			send_pthread;						//发送消息线程
	unsigned int		port = 8000;						//默认端口
	//int					sockfd;
	//struct sockaddr_in	sockaddr; 
	printf(" <main>Start ...\n");
	if(argc > 1)																//如果指定端口 则切换端口
	{
		port = atoi(argv[1]);
	}
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror(" <main>socket");
		_exit(-1);
	}
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;												//使用IPv4协议
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htons(INADDR_ANY);
	if(bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
	{
		perror(" <ReceiveMsg>bind");
		close(sockfd);
		pthread_exit(NULL);
	}
	pthread_create(&send_pthread, NULL, SendMsg, (void *)port);					//创建消息接收线程
	pthread_create(&receive_pthread, NULL, ReceiveMsg, NULL);					//创建消息接收线程
	pthread_join(send_pthread, NULL);											//等待发送线程结束(““quit”)
	pthread_cancel(receive_pthread);
	pthread_join(receive_pthread, NULL);
	printf(" <main>Stop\n");
	return 0;
}

