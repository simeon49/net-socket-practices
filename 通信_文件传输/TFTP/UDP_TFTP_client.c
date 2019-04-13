/* ************************************************************************
 *       Filename:  UDP_TFTP_client.c (文件下载 客户端)
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月18日 10时32分53秒
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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <dirent.h>

struct sockaddr_in	from_sockeaddr;							//服务器套接字结构体
int 				SetEn = 0; 								//服务器IP设置使能标志 ）0:未设置IP 
int					tftp_sockfd;							//套接字


//========================================================================
//	语法格式：	void CloseFd(int fd)
//	实现功能：	关闭文件fd
//	参数：		fd:需要关闭的文件描述符
//	返回值：	成功: 0  失败: -1 
//========================================================================
int CloseFd(int fd)
{
	if(close(fd) == -1)
	{
		perror(" \e[31m<CloseFd>%d\e[0m");
		return(-1);
	}
	return 0;
}

//========================================================================
//	语法格式：	int TFTP_GetServerIP(int argc, char *argv[])
//	实现功能：	获取服务器IP
//	参数：		int argc, char *argv[]
//	返回值：	成功: 0  失败: -1 
//========================================================================
int TFTP_GetServerIP(int argc, char *argv[])
{
	if(argc != 1)
	{
		printf(" \e[31m<TFTP_GetServerIP>Need server IP\e[0m\n");
		return (-1);
	}
	if(inet_pton(AF_INET, argv[0], &from_sockeaddr.sin_addr.s_addr) == 0)
	{
		perror(" \e[31m<TFTP_GetServerIP>inet_pton\e[0m\n");
		return (-1);
	}
	printf(" \e[33mServer IP[\e[34m%s\e[33m] Set Successfull!\e[0m\n", argv[0]);
	SetEn = 1;
	return 0;
}

//========================================================================
//	语法格式：	int TFTP_DownLoad(int argc, char *argv[])
//	实现功能：	从服务器下载文件
//	参数：		int argc, char *argv[]
//	返回值：	成功: 0  失败: -1 
//========================================================================
int TFTP_DownLoad(int argc, char *argv[])
{
	int					file_fd;							//文件描述符 	（下载文件）
	char				file_name[100];						//文件名		（下载文件）
	socklen_t			fromlen = sizeof(struct sockaddr);
	char				from_ip[INET_ADDRSTRLEN];			//TFTP服务器IP
	struct sockaddr_in	ask_sockaddr;						//TFTP服务器应答套接字结构体
	char				buf[1024];
	int					count;								//每次发送/接收的字节数 （buf）
	unsigned short int	number;								//块编号（数据包序号）
	char *				p_temp;
	//查看是否设置已设置服务器IP
	if(SetEn == 0)
	{
		printf(" \e[31m<DownLoad>Set Server IP first!\e[0m\n");
		return (-1);
	}
	//需要下载的文件名
	if(argc != 1)
	{
		printf(" \e[31m<DownLoad>Need file name\e[0m\n");
		return (-1);
	}
	strncpy(file_name, argv[0], sizeof(file_name));
	from_sockeaddr.sin_port = htons(69);														//TFTP服务器默认端口
	inet_ntop(AF_INET, &from_sockeaddr.sin_addr.s_addr, from_ip, sizeof(from_ip));
	printf(" \e[33mSever: \e[34m[%s]\n \e[33mFileName: \e[34m%s\e[0m\n", from_ip, file_name);
	//向服务器提交下载请求
	memset(buf, '\0', sizeof(buf));
	count = sprintf(buf, "%c%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c", 00 , 01, file_name, 00, "octet", 00, "blksize", 00, "512", 00, "tsize", 00, "0", 00, "timeout", 00, "5", 00);
	//count = sprintf(buf, "%c%c%s%c%s%c", 00 , 01, file_name, 00, "octet", 00);
	if( (count = sendto(tftp_sockfd, buf, count, 0, (struct sockaddr *)&from_sockeaddr,sizeof(from_sockeaddr) )) ==-1 )
	{
		perror(" \e[31m<DownLoad>sendto\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);	
	}
	//接收服务器的回应
	if( (count = recvfrom(tftp_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&ask_sockaddr, &fromlen)) == -1)
	{
		perror(" \e[31m<DownLoad>recvfrom\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);
	}
	if(buf[1] == 0x05)								//下载失败
	{
		printf(" \e[31m<DownLoad>error:\e[0m%s\n", buf + 4);
		CloseFd(tftp_sockfd);
		return(-1);
	}
	//向服务器发送确认信息
	count = sprintf(buf, "%c%c%c%c", 00, 04, 00, 00);
	if( (count = sendto(tftp_sockfd, buf, count, 0, (struct sockaddr *)&ask_sockaddr,sizeof(ask_sockaddr) )) == -1 )
	{
		perror(" \e[31m<DownLoad>sendto\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);	
	}
	//创建下载文件
	if( (file_fd = open(file_name, O_CREAT|O_WRONLY, 0777))== -1 )
	{
		perror(" \e[31m<DownLoad>open\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);
	}
	//接收文件内容
	number = 0;										//数据包序号
	while(1)
	{
		if( (count = recvfrom(tftp_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&ask_sockaddr, &fromlen)) == -1)
		{
			perror(" \e[31m<DownLoad>recvfrom\e[0m");
			return(-1);
		}
		if(buf[1] == 0x05)							//下载失败
		{
			printf(" \e[31m<DownLoad>error:\e[0m%s\n", buf + 4);
			CloseFd(file_fd);
			unlink(file_name);						//删除下载文件
			CloseFd(tftp_sockfd);
			return(-1);
		}
		//确认数据包序号
		p_temp = &buf[2];
		if((*(unsigned short int *)p_temp) == htons(number + 1) || (*(unsigned short int *)p_temp) == htons(number))	//当收到的数据包号没有增加（即 发送的ACK丢失）则重新发送ACK
		{
			if( (*(unsigned short int *)p_temp) == htons(number + 1) )						//将新的数据包内容写入下载文件中
			{
				number ++;
				write(file_fd, buf+4, count-4);
				//printf(" \e[33mGetPart<\e[34m%5d\e[33m> size=\e[34m%3d\e[0m\n", number, count - 4);
				if(count - 4 < 512)
				{
					printf(" \e[33mFile: \e[34m%s\e[33m Size=\e[34m%.2f\e[33m(KB) Download Successfull!\e[0m\n", file_name, (number * 512 + count -4)/1024.0);
					break;
				}
			}
			buf[0] = 00;
			buf[1] = 04;
			if( (count = sendto(tftp_sockfd, buf, 4, 0, (struct sockaddr *)&ask_sockaddr,sizeof(ask_sockaddr) )) ==-1 )
			{
				perror(" \e[31m<DownLoad>sendto\e[0m");
				return(-1);	
			}
		}
	}
	//保存并关闭下载文件
	CloseFd(file_fd);
	return 0;
}

//========================================================================
//	语法格式：	int TFTP_UpLoad(int argc, char *argv[])
//	实现功能：	将文件上传到服务器
//	参数：		int argc, char *argv[]
//	返回值：	成功: 0  失败: -1 
//========================================================================
int TFTP_UpLoad(int argc, char *argv[])
{
	int					file_fd;							//文件描述符 	（下载文件）
	char				file_name[100];						//文件名		（下载文件）
	socklen_t			fromlen = sizeof(struct sockaddr);
	char				from_ip[INET_ADDRSTRLEN];			//TFTP服务器IP
	struct sockaddr_in	ask_sockaddr;						//TFTP服务器应答套接字结构体
	char				buf[1024];
	char				buf0[1024];
	int					count;								//每次发送/接收的字节数 （buf）
	unsigned short int	number;								//块编号（数据包序号）
	char *				p_temp;
	//查看是否设置已设置服务器IP
	if(SetEn == 0)
	{
		printf(" \e[31m<TFTP_UpLoad>Set Server IP first!\e[0m\n");
		return (-1);
	}
	//需要上传的文件名
	if(argc != 1)
	{
		printf(" \e[31m<TFTP_UpLoad>Need file name\e[0m\n");
		return (-1);
	}
	strncpy(file_name, argv[0], sizeof(file_name));
	inet_ntop(AF_INET, &from_sockeaddr.sin_addr.s_addr, from_ip, sizeof(from_ip));
	printf(" \e[33mSever: \e[34m[%s]\n \e[33mFileName: \e[34m%s\e[0m\n", from_ip, file_name);
	//向服务器提交上传请求
	memset(buf, '\0', sizeof(buf));
	//count = sprintf(buf, "%c%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c%s%c", 00 , 02, file_name, 00, "octet", 00, "blksize", 00, "512", 00, "tsize", 00, "0", 00, "timeout", 00, "5", 00);
	count = sprintf(buf, "%c%c%s%c%s%c", 00 , 02, file_name, 00, "octet", 00);
	if( (count = sendto(tftp_sockfd, buf, count, 0, (struct sockaddr *)&from_sockeaddr, sizeof(from_sockeaddr) )) ==-1 )
	{
		perror(" \e[31m<TFTP_UpLoad>sendto\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);	
	}
	/*
	//接收服务器的回应
	if( (count = recvfrom(tftp_sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&ask_sockaddr, &fromlen)) == -1)
	{
		perror(" \e[31m<TFTP_UpLoad>recvfrom\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);
	}
	if(buf[1] != 0x06)								//上传失败
	{
		printf(" \e[31m<TFTP_UpLoad>error:\e[0m%s\n", buf + 4);
		CloseFd(tftp_sockfd);
		return(-1);
	}
	//向服务器发送确认信息
	count = sprintf(buf, "%c%c%c%c", 00, 04, 00, 00);
	if( (count = sendto(tftp_sockfd, buf, count, 0, (struct sockaddr *)&ask_sockaddr, sizeof(ask_sockaddr) )) == -1 )
	{
		perror(" \e[31m<TFTP_UpLoad>sendto\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);	
	}*/
	//打开上传文件
	if( (file_fd = open(file_name, O_RDONLY)) == -1 )
	{
		perror(" \e[31m<TFTP_UpLoad>open\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);
	}
	//发送数据包
	number = 0;										//数据包序号
	while(1)
	{
		if( (count = recvfrom(tftp_sockfd, buf0, sizeof(buf0), 0, (struct sockaddr *)&ask_sockaddr, &fromlen)) == -1)
		{
			perror(" \e[31m<TFTP_UpLoad>recvfrom\e[0m");
			return(-1);
		}
		if(buf0[1] == 0x05)							//上传失败
		{
			printf(" \e[31m<TFTP_UpLoad>error:\e[0m%s\n", buf0 + 4);
			CloseFd(file_fd);
			CloseFd(tftp_sockfd);
			return(-1);
		}
		//发送数据包
		p_temp = &buf0[2];
		if((*(unsigned short int *)p_temp) == htons(number) || (*(unsigned short int *)p_temp) == htons(number - 1))	//当收到的数据包号没有增加（即 发送的ACK丢失）则重新发送ACK
		{
			if( (*(unsigned short int *)p_temp) == htons(number) )						//将新的数据包内容写入下载文件中
			{
				number ++;
				count = read(file_fd, buf+4, 512);
				//printf(" \e[33mGetPart<\e[34m%5d\e[33m> size=\e[34m%3d\e[0m\n", number, count);
			}
			buf[0] = 00;
			buf[1] = 03;
			*(unsigned short int *)(buf + 2) = htons(number);
			if( (count = sendto(tftp_sockfd, buf, count + 4, 0, (struct sockaddr *)&ask_sockaddr, sizeof(ask_sockaddr) )) == -1 )
			{
				perror(" \e[31m<TFTP_UpLoad>sendto\e[0m");
				CloseFd(file_fd);
				return(-1);	
			}
			if(count - 4 < 512)
			{
				printf(" \e[33mFile: \e[34m%s\e[33m Size=\e[34m%.2f\e[33m(KB) UpLoad Successfull!\e[0m\n", file_name, (number * 512 + count -4)/1024.0);
				break;
			}
		}
	}
	if( (count = recvfrom(tftp_sockfd, buf0, sizeof(buf0), 0, (struct sockaddr *)&ask_sockaddr, &fromlen)) == -1)
	{
		perror(" \e[31m<TFTP_UpLoad>recvfrom\e[0m");
		return(-1);
	}
	//关闭文件
	CloseFd(file_fd);
	return 0;
}

/**********************************************
函数原型: int ls_fun(int argc, char *argv[])
函数接口: int argc, char *argv[]
函数返回: 成功: 0  失败: -1 
函数功能: 显示当前文件夹内容
**********************************************/
int ls_fun(int argc, char *argv[])
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
	return 0; 
}
//帮助文档打印消息
char *help= "*********************************************************************************\n"\
			"* set  [ServerIP]					    :设置服务器IP地址	*\n"\
			"* upload  [filenum]					:上传文件		*\n"\
			"* download  [filenum]					:下载文件		*\n"\
			"* help							        :帮助			*\n"\
			"* ls						            :查看当前目录文件名	*\n"\
			"* clear							    :清屏			*\n"\
			"* exit/quit						    :退出			*\n"\
			"**********************************************************************************\n"
			;

/**********************************************
函数原型: int help_fun(int argc, char *argv[])
函数接口: int argc, char *argv[]
函数返回: int
函数功能: 打印帮助信息
**********************************************/
int help_fun(int argc, char *argv[])
{
	printf("%s",help);
	return 0; 
}

/**********************************************
函数原型: int clear_fun(int argc, char *argv[])
函数接口: int argc, char *argv[]
函数返回: int
函数功能: 清屏
**********************************************/
int clear_fun(int argc, char *argv[])
{
	write(1,"\033[2J",4);   //清屏
	write(1,"\033[0;0H",6); //将光标定位在左上角
	return 0;
}	
#define GREEN 	"\e[32m"   //shell打印显示绿色
#define RED     "\e[31m"   //shell打印显示红色
#define PRINT(X,Y) {	write(1,Y,5);  \
					printf(X);   \
					fflush(stdout); \
				    write(1,"\e[0m",4); \
				 }


//函数指针			
typedef int (*FUN) (int argc, char *argv[]);

typedef struct _cmd
{
	char *name;
	FUN fun;
}CMD;

//定义结构体数组
CMD cmdlist[] = {
					{"set", TFTP_GetServerIP},
					{"upload", TFTP_UpLoad},
					{"download", TFTP_DownLoad},
					{"help", help_fun},
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
			cmdlist[i].fun(argc-1, argv+1);			//执行命令
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

int main(int argc, char *argv[])
{	
	char				from_ip[INET_ADDRSTRLEN];			//TFTP服务器IP
	//创建socket套接字
	if( (tftp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 )
	{
		perror(" \e[31m<DownLoad>socket\e[0m");
		CloseFd(tftp_sockfd);
		return(-1);
	}
	memset(&from_sockeaddr, 0, sizeof(from_sockeaddr));
	from_sockeaddr.sin_family = AF_INET;
	from_sockeaddr.sin_port = htons(69);														//TFTP服务器默认端口
	inet_pton(AF_INET, from_ip, &from_sockeaddr.sin_addr.s_addr);
	
	interface();
	/*
	if(TFTP_GetServerIP(1, argv + 1) == -1)
	{
		return -1;
	}
	if(TFTP_DownLoad(1, argv + 2) == -1)
	{
		return -1;
	}
	char *p[] = {"ASCII0.bmp"};
	if(TFTP_UpLoad(1, p) == -1)
	{
		return -1;
	}*/
	CloseFd(tftp_sockfd);
	return 0;
}


