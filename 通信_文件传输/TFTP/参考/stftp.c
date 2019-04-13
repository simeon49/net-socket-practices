/********************************************
file:    stftp.c
author:  
data:    2011-09-29
version: v1.0
function: 支持TFTP的上传,下载
running:  ./tftp

Maintainer:  v1.1 2011-10-28
			1. 添加注释
			2. 优化代码
			意见:ls显示当前目录文件时,能指示出当前文件大小

Maintainer:  v1.2 2011-11-4
			1. 添加清屏功能
			2. 修改上传部分BUG

Maintainer:  v1.3 2012-03-06
			1. 修复传大文件失败的问题
			2. 帮助打印中添加清屏标示
*********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>


#define GREEN 	"\e[32m"   //shell打印显示绿色
#define RED     "\e[31m"   //shell打印显示红色
#define PRINT(X,Y) {	write(1,Y,5);  \
					printf(X);   \
					fflush(stdout); \
				    write(1,"\e[0m",4); \
				 }

		 
struct sockaddr_in addr;

int SetEn = 0; //服务器IP设置使能标志
int udpfd = 0; //UDP文件描述符
	
//帮助文档打印消息
char *help= "*********************************************************************************\n"\
			"* set  [ServerIP]					:设置服务器IP地址	*\n"\
			"* put  [filenum]					:上传文件		*\n"\
			"* get  [filenum]					:下载文件		*\n"\
			"* help							:帮助			*\n"\
			"* ls						        :查看当前目录文件名	*\n"\
			"* clear							:清屏			*\n"\
			"* exit/quit						:退出			*\n"\
			"**********************************************************************************\n"
			;

//函数指针			
typedef void (*FUN) (int argc, char *argv[]);

typedef struct _cmd
{
	char *name;
	FUN fun;
}CMD;

/**********************************************
函数原型: void clear_fun(int argc, char *argv[])
函数接口: int argc, char *argv[]
函数返回: void
函数功能: 清屏
**********************************************/
void clear_fun(int argc, char *argv[])
{
	write(1,"\033[2J",4);   //清屏
	write(1,"\033[0;0H",6); //将光标定位在左上角
	return;
}	

/**********************************************
函数原型: void help_fun(int argc, char *argv[])
函数接口: int argc, char *argv[]
函数返回: void
函数功能: 打印帮助信息
**********************************************/
void help_fun(int argc, char *argv[])
{
	printf("%s",help);
	return;
}

/**********************************************
函数原型: void exit_fun(int argc,char *argv[])
函数接口: int argc, char *argv[]
函数返回: void
函数功能: 关闭udp套接字,退出函数
**********************************************/
void exit_fun(int argc,char *argv[])
{
	close(udpfd);
	exit(1);
}

/**********************************************
函数原型: void set_fun(int argc,char *argv[])
函数接口: int argc, char *argv[]
函数返回: void
函数功能: 使能服务器IP地址设置成功标志,并设置套接字地址结构体
**********************************************/
void set_fun(int argc,char *argv[])
{
	inet_pton(AF_INET,argv[1],&addr.sin_addr);
	SetEn = 1; //如果设置IP后才可以使用get,put指令
	
	PRINT("Set Server IP Successful\n",RED);	
	return;
}

/**********************************************
函数原型: void get_fun(int argc,char *argv[])
函数接口: int argc, char *argv[]
函数返回: void
函数功能: 从TFTP服务器上获取文件
**********************************************/
void get_fun(int argc,char *argv[])
{
	if(SetEn == 0)
	{
		PRINT("Please Set The Server IP\n",RED);
		return;
	}
	int fd = 0;
	unsigned short p_num = 0;
	char buf[516] = "";
	volatile unsigned char cmd = 0;
	volatile unsigned int len = 0;
	
	struct sockaddr_in cliaddr;
	socklen_t addrlen = sizeof(cliaddr);
	

	//构造下载请求,argv[1]为文件名
	len = sprintf(buf,"%c%c%s%c%s%c", 0, 1, argv[1],0, "octet", 0);	//发送读数据包
	
	sendto(udpfd, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
	
	if( (fd = open(argv[1],O_WRONLY|O_CREAT,0666)) < 0 )
	{
		perror("open error");
		exit(-1);
	}

	do{
		printf("the do_while test\n");
		//接收服务器发送的内容
		len = recvfrom(udpfd, buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, &addrlen);
		cmd = buf[1];
		printf("the len = %d,cmd = %d\n",len,cmd);
		if( cmd == 3 )	//是否为数据包
		{
			//包编号是否和上次相等		
			if( (unsigned short)(p_num+1) == ntohs(*(unsigned short*)(buf+2) ))
			{
				write(fd, buf+4, len-4);
				p_num = ntohs(*(unsigned short*)(buf+2));
				printf("recv:%d\n", p_num);	//十进制方式打印包编号
			}
			buf[1] = 4;
			sendto(udpfd, buf, 4, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
		}
		else if( cmd == 6 ) //是否为OACK回应
		{
			 buf[0] = 0; buf[1] = 4; buf[2] = 0; buf[3] = 0;
			 sendto(udpfd, buf, 4, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
			 continue;
		}
		else if( cmd==5 ) //是否为错误应答
		{
			close(fd);
			close(udpfd);
			unlink(argv[1]);
			printf("error:%s\n",buf+4);
			return;
		}		
	}while((len == 516)||(cmd == 6)); //如果收到的数据小于516则认为出错
	
	close(fd);
	PRINT("Download File is Successful\n",RED);
	return;
}

/**********************************************
函数原型: void put_fun(int argc, char *argv[])
函数接口: int argc, char *argv[]
函数返回: void
函数功能: 向TFTP服务器上传文件.(不支持LINUX下的TFTP服务器)
**********************************************/
void put_fun(int argc, char *argv[])
{	
	if(SetEn == 0)
	{
		PRINT("Please Set The Server IP\n",RED);
		return;
	}
	
	int fd = 0;
	unsigned short p_num = 0;
	char buf[516] = "";
	unsigned char cmd = 0;
	
	struct sockaddr_in cliaddr;
	socklen_t addrlen = sizeof(cliaddr);
	
	unsigned int len = 0;
	int readlen = 0;
	
	if( (fd = open(argv[1],O_RDONLY)) < 0 )
	{
		perror("open error");
		exit(-1);
	}
	
	//构造下载请求,argv[1]为文件名
	len = sprintf(buf,"%c%c%s%c%s%c", 0, 2, argv[1],0, "octet", 0);	//发送写数据包
	sendto(udpfd, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
	
	do{
		//接收服务器发送的内容
		len = recvfrom(udpfd, buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, &addrlen);
		cmd = buf[1];
		if( cmd == 4 )	//是否为ACK包
		{
			p_num = ntohs(*(unsigned short*)(buf+2));

			readlen = read(fd, buf+4, 512);	
			printf("send:%d,%d\n", p_num,readlen );	//十进制方式打印包编号
			buf[1] = 3;
			(*(unsigned short *)(buf+2)) = htons(p_num + 1);
			sendto(udpfd, buf, readlen + 4, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr));
		}
		else if( cmd==5 ) //是否为错误应答
		{
			close(fd);
			close(udpfd);
			printf("error:%s\n",buf+4);
			return;
		}		
	}while(readlen == 512); //如果读到的数据小于512则认为出错

	len = recvfrom(udpfd, buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, &addrlen);
	
	close(fd);
	PRINT("Upload File is Successful\n",RED);
	return;
}

/**********************************************
函数原型: void ls_fun(int argc, char *argv[])
函数接口: int argc, char *argv[]
函数返回: void
函数功能: 显示当前文件夹内容
**********************************************/
void ls_fun(int argc, char *argv[])
{
	DIR *dp;
	char *dir = ".";
	struct dirent *dirp;
	if( (dp = opendir (dir)) == NULL)
		printf ("can't open %s\n", dir);
	
	while( (dirp = readdir (dp)) != NULL)
		printf ("%s  ", dirp->d_name);
	printf("\n");	
	closedir (dp);
	return;
}

//定义结构体数组
CMD cmdlist[] = {
					{"set", set_fun},
					{"put", put_fun},
					{"get", get_fun},
					{"help", help_fun},
					{"exit", exit_fun},
					{"quit", exit_fun},
					{"ls", ls_fun},
					{"clear", clear_fun}
				};
				
/**********************************************
函数原型: int exec_cmd(char *cmd)
函数接口: char *cmd; 输入的指令
函数返回: 找到指令返回0,没有找到指令返回-1
函数功能: 解析输入的指令,并对不同的指令执行不同的函数
**********************************************/				
int exec_cmd(char *cmd)
{
	char *argv[10] = {NULL};
	int argc = 0;
	int i = 0;
	if(strlen(cmd) == 0)
		return 0;
		
	argv[0] = cmd;
	while((argv[argc]=strtok(argv[argc], " \t")) != NULL) 
		argc++;

	for (i = 0; i < sizeof(cmdlist)/sizeof(CMD); i++)
	{
		//查找命令
		if (strcmp(cmdlist[i].name, argv[0]) == 0)
		{
			cmdlist[i].fun(argc, argv);			//执行命令
			return 0;
		}
	}	
	return -1;
}	

/**********************************************
函数原型: void interface()
函数接口: NULL
函数返回: NULL
函数功能: 循环等待输入,并将输入的数据传送给exec_cmd函数进行解析
**********************************************/	
void interface()
{
	char inputBuf[100] = "";
	while(1)
	{
		PRINT("send>",GREEN);
		bzero(inputBuf,sizeof(inputBuf));
		fgets(inputBuf,sizeof(inputBuf),stdin);
		inputBuf[strlen(inputBuf) - 1] = 0;
		if(exec_cmd(inputBuf) < 0)
		{
			PRINT( "Command Not Found!\n",RED);
			continue;
		}
	}
	return;
}

/**********************************************
函数原型: int main(int argc, char *argv[])
函数接口: NULL
函数返回: NULL
函数功能: 创建UDP socket并调用interface()等待用户输入
**********************************************/	
int main(int argc, char *argv[])
{
	//对套接字地址结构体进行初始化
	addr.sin_family = AF_INET;
	addr.sin_port = htons(69);  //TFTP默认端口69
	
	//创建UDP 套接字
	if( (udpfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
	{
		close(udpfd);
		perror("socket error");
		exit(-1);
	}
	
	printf("%s",help); //打印help信息
	PRINT( "Please Set The Server IP\n",RED);
	
	interface(); //判断输入是否正确
	return 0;
}
