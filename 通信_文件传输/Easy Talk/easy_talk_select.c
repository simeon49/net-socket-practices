/* ************************************************************************
 *       Filename:  select.c（UDP协议）（单进程 单线程）
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月16日 09时50分12秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>

#define		MAX2(a,b) (((a) > (b))?(a):(b))
#define		ERROR_EXIT	-1							//函数因错误而退出

//========================================================================
//	语法格式：	int ReceiveAMsg(int sockfd)
//	实现功能：	从sockfd 中读取一个网络消息
//	参数：		sockfd ：sockeet文件描述符
//	返回值：	成功：0   失败：-1
//========================================================================
int ReceiveAMsg(int sockfd)
{
	int					rec;
	char				buf[1024];
	struct sockaddr_in	from_sockaddr;
	socklen_t			from_sockaddr_len = sizeof(from_sockaddr);
	char				from_ip[INET_ADDRSTRLEN];
	if((rec = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&from_sockaddr, &from_sockaddr_len)) == -1)
	{
		perror(" \e[31m<ReceiveAMsg>recvfrom\e[0m");
		return ERROR_EXIT;
	}
	inet_ntop(AF_INET, &from_sockaddr.sin_addr.s_addr, from_ip, sizeof(from_ip));
	printf("\r\e[32m[From %s:%d] %s\e[0m", from_ip, ntohs(from_sockaddr.sin_port), buf);
	printf(">");
	fflush(stdout);
	return 0;
}

#define		CMD_QUIT	1							//退出
#define		CMD_SENDTO	2							//指定发送目标IP
//========================================================================
//	语法格式：	int Analyse(char buf[])
//	实现功能：	分析用户输入 并处理
//	参数：		buf：用户输入字符
//	返回值：	操作类型
//========================================================================
int Analyse(char buf[])
{
	//指令集合
	static  char *cmd_buf[] = 	{"[To]",\									//指定发送目标IP		
								"[Quit]"};									//退出程序
	if(strncmp(buf, cmd_buf[0], strlen(cmd_buf[0])) == 0)
	{
		int			i = 0;
		int			j = 0;
		while(buf[i] < '0' || buf[i] > '9')
			i ++;
		while(buf[i] != '\n')
		{
			buf[j] = buf[i];
			i ++;
			j ++;
		}
		buf[j] = '\0';
		//printf(" <Analyse>IP;%s,len=%d\n", buf, strlen(buf));
		return CMD_SENDTO;
	}
	else if(strncmp(buf, cmd_buf[1],strlen(cmd_buf[1])) == 0)
	{
		return CMD_QUIT;
	}
	else
		return 0;
}


//========================================================================
//	语法格式：	int SendAMsg(int sockfd, struct sockaddr_in *p_to_sockaddr)
//	实现功能：	接收用户输入 并向指定IP（端口） 发送消息
//	参数：		sockfd：socket描述符   p_to_sockaddr：指向消息发送目标 socket结构体
//	返回值：	成功：0  失败：-1
//========================================================================
int SendAMsg(int sockfd, struct sockaddr_in *p_to_sockaddr)
{
	char		buf[1024];
	int			rec;
	memset(buf, '\0', sizeof(buf));
	fgets(buf, sizeof(buf), stdin);
	switch(Analyse(buf))
	{
		case CMD_QUIT:
			return CMD_QUIT;
			break;
		case CMD_SENDTO:
			if(inet_pton(AF_INET, buf, (struct sockaddr *)&(p_to_sockaddr->sin_addr.s_addr)) == 0)
			{
				perror(" \e[31m<SendAMsg>inet_pton\e[0m");
				return (ERROR_EXIT);
			}
			memset(buf, '\0', sizeof(buf));
			strcpy(buf, "[RING]\n");
			break;
		case ERROR_EXIT:
			return (CMD_QUIT);
		default:
			break;
	}
	if((rec = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)(p_to_sockaddr), sizeof(struct sockaddr))) == -1)
	{
		perror(" \e[31m<SendAMsg>sendto\e[0m");
		return (ERROR_EXIT);
	}
	printf(">");
	fflush(stdout);
	return 0;
}

//========================================================================
//	语法格式：	void EasyTalk(unsigned int port)
//	实现功能：	建立UDP通讯 使用select()监听套接字socket 与标准输入 实现单线程通讯
//	参数：		port:(UDP本机端口  目标UDP端口)
//	返回值：	无
//========================================================================
void EasyTalk(unsigned int port)
{
	int					sockfd;
	struct sockaddr_in	sockaddr;
	struct sockaddr_in	to_sockaddr;
	fd_set				readfds;						
	int					res;
	//struct timeval		timeout;
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror(" \e[31m<EasyTalk>socket\e[0m");
		_exit(EXIT_FAILURE);
	}
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);							//接受本机所以可用IP
	if(bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1)
	{
		perror(" \e[31m<EasyTalk>bind\e[0m");
		if(close(sockfd) == -1)
			perror(" \e[31m<EasyTalk>close(socked)\e[0m");
		_exit(EXIT_FAILURE);
	}
	memset(&to_sockaddr, 0, sizeof(to_sockaddr));
	to_sockaddr.sin_family = AF_INET;
	to_sockaddr.sin_port = htons(port);
	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);													//监听套接字 是否可读 （接收到消息）
	FD_SET(0, &readfds);														//监听标准输入		（用户输入）
	printf(">");
	fflush(stdout);
	while((res = select( MAX2(0, sockfd) + 1, &readfds, NULL, NULL, NULL)) != -1)
	{
		if(FD_ISSET(sockfd, &readfds))
		{
			//printf(" \e[34m<EasyTalk>Receive a msg\e[0m\n");
			if(ReceiveAMsg(sockfd) == CMD_QUIT)
				break;
		}
		else if(FD_ISSET(0, &readfds))
		{
			//printf(" \e[34m<EasyTalk>Sand a msg\e[0m\n");
			if(SendAMsg(sockfd, &to_sockaddr) == CMD_QUIT)
				break;
		}
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);													//监听套接字 是否可读 （接收到消息）
		FD_SET(0, &readfds);														//监听标准输入		（用户输入）
	}
	if(res == -1)
	{
		perror(" \e[31m<EasyTalk>select\e[0m");
		if(close(sockfd) == -1)
			perror(" \e[31m<EasyTalk>close(socked)\e[0m");
		_exit(EXIT_FAILURE);
	}
	if(close(sockfd) == -1)
	{
		perror(" \e[31m<EasyTalk>close(sockfd)");
		_exit(EXIT_FAILURE);
	}
}


int main(int argc, char *argv[])
{
	unsigned int		port = 8000;
	if(argc > 1)
	{
		port = atoi(argv[1]);
	}
	printf(" \e[1m\e[32m<main>Start EasyTalk...\e[0m\n");
	EasyTalk(port);
	return 0;
}


