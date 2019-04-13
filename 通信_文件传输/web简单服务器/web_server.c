/* ************************************************************************
 *       Filename:  web_server.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月14日 11时25分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
unsigned short			port = 80;				//开80为网路服务器端口

//========================================================================
//	语法格式：	void Server(int *p_sockfd)
//	实现功能：	服务端口 实现向客户端发送从客户端接收到得信息
//	参数：		p_sockfd指向 客户端的套接字
//	返回值：	无
//========================================================================
void * Server(void *arg)
{
	int			sockfd;
	int			html_fd;
	int			count;
	char		buf[1024];
	char		html_name[256];		
	int			len;
	char head[]="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
	char err[]=	"HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<HTML><BODY>File not found</BODY></HTML>";
	sockfd = (int)arg;
	memset(buf, '\0', sizeof(buf));
	printf(" <Server> Running\n");
	if(recv(sockfd, buf, sizeof(buf), 0) <= 0)
	{
		printf(" <Server>Link down\n");
		return NULL;
	}
	//while(recv(sockfd, buf, sizeof(buf), 0) > 0)
	//{
	//	printf(" >%s\n",buf);
	//}
	//printf("\n\n>>>>>%s\n\n",buf);
	if(strstr(buf, "GET") != NULL)
	{
		memset(html_name, '\0', sizeof(html_name));										//解析客户端发送的命令
		strcpy(html_name,"./html/");													//如果命令中有 "GET"则查找对应文件
		len = strlen(html_name);
		sscanf(buf, "GET /%[^ ]/",html_name + len);
		if(strlen(html_name) == len)													//默认打开index.html
			strcat(html_name, "index.html");
		//sscanf(buf, "GET /%[^ ]",html_name);
		//printf(" <Server> Get:%s\n",html_name);
		if((html_fd = open(html_name, O_RDONLY)) == -1)									//未找到相应.html文件 则发送失败信息
		{
			send(sockfd, err, strlen(err), 0);
			return NULL;
		}
		else																			//找到相应.html文件 则发送成功信息 与当前.html文件内容
		{
			send(sockfd, head, strlen(head), 0);
			while((count = read(html_fd, buf, sizeof(buf))) >0 )						//发送数据
			{
				send(sockfd, buf, count, 0);
			}
			close(html_fd);																//关闭html文件
		}
	}
	//printf(" <Server>Link down\n");
	
	close(sockfd);																		//关闭客户套接字(为什么必须关闭)
	return NULL;
}

//========================================================================
//	语法格式：	int main(int argc, char *argv[])
//	实现功能：	建立套接字 绑定套接字（端口 ip 网络协议） 监听 创建响应客户
//				端的处理线程
//	参数：		int argc, char *argv[] 
//	返回值：	0
//========================================================================
int main(int argc, char *argv[])
{
	int				listen_sockfd;						//监听套结字
	if(argc >1)
	{
		port = atoi(argv[1]);
	}
	if((listen_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)									//创建监听套接字
	{
		perror(" <main>socket");
		_exit(-1);
	}
	struct sockaddr_in	listen_sockaddr;
	memset(&listen_sockaddr, 0, sizeof(listen_sockaddr));
	listen_sockaddr.sin_family = AF_INET;														//IPV4协议
	listen_sockaddr.sin_port = htons(port);														//设置端口为port
	listen_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);										//使用本地所有可用IP
	if(bind(listen_sockfd, (struct sockaddr *)&listen_sockaddr, sizeof(listen_sockaddr)) == -1)	//绑定套接字
	{
		perror(" <main>bind");
		close(listen_sockfd);
		_exit(-1);
	}
	if(listen(listen_sockfd, 10) == -1)															//监听  最大同时响应服务为10个
	{
		perror(" <main>listen");
		close(listen_sockfd);
		_exit(-1);
	}
	while(1)
	{
		pthread_t			server_pthread;				//服务线程
		int					client_sockfd;				//客户端套接字
		struct sockaddr_in	client_sockaddr;			//客户套接字IPv4
		char				client_IP[INET_ADDRSTRLEN];	//客户端ip
		socklen_t			addrlen;					//accept使用 struct sockaddr的长度
		addrlen	= sizeof(client_sockaddr);
		if((client_sockfd = accept(listen_sockfd, (struct sockaddr *)&client_sockaddr, &addrlen)) == -1)	//等待客户访问
		{
			perror(" <main>accept");
			close(listen_sockfd);
			_exit(-1);
		}
		printf(" >>>>>>>>>>>>>%d\n",client_sockfd);
		inet_ntop(AF_INET, &client_sockaddr.sin_addr.s_addr, client_IP, sizeof(client_IP));		//显示客户IP
		printf(" >>Client IP:%s\n",client_IP);
		if(pthread_create(&server_pthread, NULL, Server, (void *)client_sockfd) != 0)			//创建线程服务客户
		{
			printf(" <main>error:pthread_create\n");
			close(client_sockfd);
			close(listen_sockfd);
			_exit(-1);
		}
		if(pthread_detach(server_pthread) != 0)													//抛弃子线程
		{
			printf(" <main>error:pthread_datach\n");
			close(client_sockfd);
			close(listen_sockfd);
			_exit(-1);
		}
	}
	close(listen_sockfd);
	return 0;
}





