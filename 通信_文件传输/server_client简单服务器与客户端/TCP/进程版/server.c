/* ************************************************************************
 *       Filename:  server.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月13日 20时18分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

unsigned short	port = 80;		//端口号

//========================================================================
//	语法格式：	void Server(int *p_sockfd)
//	实现功能：	服务端口 实现向客户端发送从客户端接收到得信息
//	参数：		p_sockfd指向 客户端的套接字
//	返回值：	无
//========================================================================
void Server(int *p_sockfd)
{
	char	buf[1024];
	while(1)
	{
		memset(buf, '\0', sizeof(buf));
		if(read(*p_sockfd, buf, sizeof(buf)) <= 0)
		{
			printf(" >link down\n");
			break;
		}
		if(write(*p_sockfd, buf, sizeof(buf)) == -1)
		{
			perror(" <Server>write\n");
			break;
		}
	}
}
//========================================================================
//	语法格式：	int main(int argc, char *argv[])
//	实现功能：	建立套接字 绑定套接字（端口 ip 网络协议） 监听 创建响应客户
//				端的处理进程
//	参数：		int argc, char *argv[] 
//	返回值：	0
//========================================================================
int main(int argc, char *argv[])
{
	int					server_sockfd;
	struct sockaddr_in	server_addr;
	if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)		//创建套接字
	{
		perror(" <main>socket\n");
		_exit(-1);
	}
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;								//设置通信协议 端口 ip
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)	//绑定套接字
	{
		close(server_sockfd);
		perror(" <main>bind");
		_exit(-1);
	}
	if(listen(server_sockfd, 10) == -1)								//监听端口
	{
		close(server_sockfd);
		perror(" <main>listen");
		_exit(-1);
	}
	while(1)
	{
		int					connect_sockfd;
		socklen_t			addrlen;
		struct sockaddr_in	connect_addr;
		int					pid;
		if((connect_sockfd = accept(server_sockfd, (struct sockaddr *)&connect_addr, &addrlen)) == -1)	//如果有客户连接到服务器建立 子进程进行服务
		{
			perror(" <main>accept");
			_exit(-1);
		}
		pid = fork();
		if(pid < 0)
		{
			close(connect_sockfd);
			perror(" <main>fork");
			_exit(-1);
		}
		else if(pid == 0)
		{
			char		conncet_ip[16];
			memset(&conncet_ip, '\0', sizeof(conncet_ip));
			inet_ntop(AF_INET, &connect_addr.sin_addr.s_addr, conncet_ip, sizeof(conncet_ip));
			printf(" >Connect IP is: %s\n",conncet_ip);
			Server(&connect_sockfd);
			close(server_sockfd);									//必须关闭 只在父进程中保留
			close(connect_sockfd);
			_exit(0);
		}
		else
		{
			close(connect_sockfd);
		}
	}
	close(server_sockfd);
	return 0;
}


