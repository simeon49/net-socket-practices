/* ************************************************************************
 *       Filename:  IPMSG.c （飞鸽的主要实现部分）
 *    Description:  还未解决的问题（无法正确接收大文件 还不是很稳定）
 *        Version:  1.0
 *        Created:  2012年06月22日 16时03分25秒
 *         Author:  Ebenezer 
 * ************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <dirent.h>
//网络相关头文件
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
//自定义头文件
#include "IPMSG_D.h"
#include "UserList.h"
#include "FileList.h"

int					user_pic = 0;		//用户头像
char				user_name[20];		//本机用户名
char				host_name[20];		//本机主机名
struct user_info 	user_list;			//用于存放在线用户信息(用户名、主机名、IP（32位网络字节序）)
struct file_info 	file_list;			//用于存放可下载文件信息(文件名、文件序号、包编号、文件大小、最后修改时间、发送者IP（32位网络字节序）)
struct file_info 	server_file_list;	//用于存放可被下载文件信息(文件名、文件序号、包编号、文件大小、最后修改时间、发送者IP（32位网络字节序）)
int					udp_sockfd;			//UDP socket
int					user_state = 1;		//用户状态 1显示在线 2显示下线

//========================================================================
//	语法格式：	int IPMSG_UDP_Send(int who, unsigned long cmd, char buf[], int buf_len)
//	实现功能：	按IPSMG协议组织发送包 并通过UDP发送（UserList_GetNetIP（）)
//	参数：		int who：接收者   int cmd：命令字   char buf[]：附加信息 int buf_len buf的长度
//	返回值：	成功: 0  失败: -1 
//========================================================================
int IPMSG_UDP_Send(int who, unsigned long cmd, char buf[], int buf_len)
{
	int					len;
	int					s_addr;
	char				str[1024];
	struct sockaddr_in	to_sockaddr;
	if(UserList_GetUserAddr(&user_list, who, &s_addr) == -1){
		printf(" \e[31m<IPMSG_UDP_Send>No Such User Number\e[0m\n");
		return -1;
	}
	memset(&to_sockaddr, 0, sizeof(to_sockaddr));
	to_sockaddr.sin_family =AF_INET;
	to_sockaddr.sin_port = htons(2425);
	to_sockaddr.sin_addr.s_addr = s_addr;
	memset(str, '\0', sizeof(str));
	if((len = sprintf(str, "1_lbt4_%d#131#1078D29A1DAF#0#0#0:%ld:%s:%s:%lu:", user_pic, time((time_t *)NULL), user_name, host_name, cmd)) < 0){
		printf(" \e[31m<IPMSG_UDP_Send>sprintf:maybe input is too larg!\n");
		return -1;
	}
	if(buf_len != 0){
		memcpy(str + len, buf, buf_len);
	}
	if(sendto(udp_sockfd, str, len + buf_len, 0, (struct sockaddr *)&to_sockaddr, sizeof(to_sockaddr)) == -1){
		//perror(" \e[31m<IPMSG_UDP_Send>sendto\e[0m");
		return -1;
	}
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_UDP_Send_Str(char buf[]) 		(调用IPMSG_UDP_Send()实现)
//	实现功能：	按IPSMG协议组织发送包 并通过UDP发送（UserList_GetNetIP（）)
//	参数：		char buf[]
//	返回值：	成功0  遇到错误 -1 
//========================================================================
int IPMSG_UDP_Send_Str(char buf[])
{
	int			who;
	char		*p_temp;
	char		str[1024];
	if(buf == NULL){
		printf(" \e[31mLess arguments!\n");
		return 0;
	}
	memset(str, '\0', sizeof(str));
	if(sscanf(buf, " %d %s", &who, str) == EOF){
		printf(" \e[31mLess arguments!\n");
		return 0;
	}
	if(str[0] != '\0'){
		p_temp = strstr(buf, str);
		IPMSG_UDP_Send(who, IPMSG_SENDMSG, p_temp, strlen(p_temp));
	}
	else{
		IPMSG_UDP_Send(who, IPMSG_SENDMSG, NULL, 0);
	}
	return 0;
}
//========================================================================
//	语法格式：	int IPMSG_UserAdd(unsigned int s_addr, char *argv[])
//	实现功能：	根据ip(s_addr) 添加新用户 或更新用户信息
//				如果消息类型为 通知上线 则回复 IPMSG_ANSENTRY
//	参数：		unsigned int s_addr, char *argv[]
//	返回值：	成功: 0  失败: -1 
//========================================================================
int IPMSG_UserAdd(unsigned int s_addr, char *argv[])
{
	int			who;
	if((who = UserList_Find(&user_list, USERLIST_F_ADDR, (void *)&s_addr)) > 0){
		UserList_UserInfoRewrite(&user_list, who, argv[2], argv[3], s_addr);
	}
	else{
		if(UserList_Add(&user_list, argv[2], argv[3], s_addr) == -1)
			return -1;
	}
	if(argv != NULL){
		if(((atol(argv[4]) & IPMSG_BR_ENTRY) == IPMSG_BR_ENTRY) && (user_state == 1)){
			IPMSG_UDP_Send(UserList_Find(&user_list, USERLIST_F_ADDR, (void *)&s_addr), IPMSG_ANSENTRY, user_name, strlen(user_name));
		}
	}
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_GetAMsg(unsigned int s_addr, char *argv[])
//	实现功能：	在屏幕上打印消息或是要求接收文件
//	参数：		unsigned int s_addr, char *argv[] 全局（user_name）
//	返回值：	成功0  遇到错误 -1 
//========================================================================
int IPMSG_GetAMsg(unsigned int s_addr, char *argv[])
{
	unsigned long	cmd;
	int				num;
	char 			name[20];
	long			size;
	long			ltime;
	int				mode;
	if(IPMSG_UserAdd(s_addr, argv) == -1)										//将用户添加到用户列表中
		return -1;
	cmd = (unsigned long)atol(argv[4]);											//查看接收的内容是文件 还是普通消息
	//printf(">>%s,%lx\n", argv[1], atol(argv[1]));
	if((cmd & IPMSG_FILEATTACHOPT) == IPMSG_FILEATTACHOPT){						//对方发送的是文件
		num = atoi(argv[5] + 1);
		sscanf(argv[5] + 1, "%d:%[^:]:%lx:%lx:%x", &num, name, &size, &ltime, &mode);
		FileList_Add(&file_list, num, name, atol(argv[1]), size, ltime, s_addr);		//保存到可下载文件列表中
		if((size/1024/1024) != 0)
			printf("\r\e[33m[From (%d) %s][(%d) FileName: \e[35m%s \e[33mFileSize: \e[35m%.3f\e[33m(MB)]\e[0m\n", UserList_Find(&user_list, USERLIST_F_ADDR, (void *)&s_addr), argv[2], num, name, (float)size/1024.0/1024.0);
		else if((size/1024) != 0)
			printf("\r\e[33m[From (%d) %s][(%d) FileName: \e[35m%s \e[33mFileSize: \e[35m%.3f\e[33m(KB)]\e[0m\n", UserList_Find(&user_list, USERLIST_F_ADDR, (void *)&s_addr), argv[2], num, name, (float)size/1024.0);
		else
			printf("\r\e[33m[From (%d) %s][(%d) FileName: \e[35m%s \e[33mFileSize: \e[35m%ld\e[33m(B)]\e[0m\n", UserList_Find(&user_list, USERLIST_F_ADDR, (void *)&s_addr), argv[2], num, name, size);
	}
	else{																		//对方发送的是普通消息
		printf("\r\e[K\e[33m[From (%d) %s]:\e[39m%s\e[0m\n", UserList_Find(&user_list, USERLIST_F_ADDR, (void *)&s_addr), argv[2], argv[5]);
	}
	printf(" \e[34m>");
	fflush(stdout);
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_UserDel(unsigned int s_addr, char *argv[])
//	实现功能：	根据ip(s_addr) 从列表中删除用户
//	参数：		unsigned int s_addr, char *argv[]
//	返回值：	0 
//========================================================================
int IPMSG_UserDel(unsigned int s_addr, char *argv[])
{
	int			who;
	if((who = UserList_Find(&user_list, USERLIST_F_ADDR, (void *)&s_addr)) > 0){
		UserList_Del(&user_list, who);
	}
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_ReceiveUdpMSGOnce(void)
//	实现功能：	通过 udp_sockfd 接收一个消息 解析调用函数处理
//	参数：		无
//	返回值：	成功: 0  失败: -1 
//========================================================================
struct CMD_receive{
	unsigned long	cmd;
	int	(* fun)(unsigned int s_addr, char *argv[]);
};
struct CMD_receive cmd_list[] = {
							{IPMSG_ANSENTRY,	IPMSG_UserAdd},			//通报在线
							{IPMSG_BR_ENTRY,	IPMSG_UserAdd},			//用户上线
							{IPMSG_BR_EXIT,		IPMSG_UserDel},			//用户退出
							{IPMSG_SENDMSG,		IPMSG_GetAMsg},			//打印语聊消息或对方准备传送文件
							{IPMSG_GETFILEDATA,	NULL}
						};
int IPMSG_ReceiveUdpMSGOnce(void)
{
	int					i;
	unsigned long		cmd;
	char				recv_buf[1024];
	char				*argv[6] = {NULL};
	int					argc = 0;
	struct sockaddr_in	from_sockaddr;
	socklen_t			from_sockaddr_len = sizeof(from_sockaddr);
	memset(recv_buf, '\0', sizeof(recv_buf));
	if(recvfrom(udp_sockfd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&from_sockaddr, &from_sockaddr_len) == -1){
		perror(" \e[31m<IPMSG_Receive_A_UDP_MSG>recvfrom\e[0m");
		return -1;
	}
	recv_buf[sizeof(recv_buf) - 1] = '\0';												//保证buf最后一位是结束符
	//printf(" Test>  %s\n", recv_buf);
	argv[argc] = strtok(recv_buf, ":");
	argc ++;
	while((argv[argc] = strtok(NULL, ":")) != NULL ){							//拆分接收到的包
		if(argc >= 5)
			break;
		argc ++;
	}
	if(argc != 5){																//忽略“':' 少于5个的包
		return 0;
	}
	argv[5] = argv[4] + strlen(argv[4]) + 1;
	cmd = (unsigned long)atol(argv[4]);											//解析命令字 并作相应处理
	//printf(" cmd = %lu\n", cmd);
	for(i = 0; i < sizeof(cmd_list)/(sizeof(struct CMD_receive)); i++){
		if((cmd & 0xff) == cmd_list[i].cmd){
			if(cmd_list[i].fun(from_sockaddr.sin_addr.s_addr, argv) == -1)		//执行遇到错误时 返回-1（失败）
				return -1;
			else{
				if(((cmd & IPMSG_SENDCHECKOPT) == IPMSG_SENDCHECKOPT) && (user_state == 1)){	//通知消息发送者消息已经收到
					sprintf(recv_buf, "%s", argv[1]);
					IPMSG_UDP_Send(UserList_Find(&user_list, USERLIST_F_ADDR, (void *)&from_sockaddr.sin_addr.s_addr), IPMSG_RECVMSG, recv_buf, strlen(recv_buf));
				}
				return 0;
			}
		}
	}
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_Help(char buf[])
//	实现功能：	打印帮助信息
//	参数：		无
//	返回值：	0 
//========================================================================
int IPMSG_Help(char buf[])
{
	printf("\e[32m\t*********************************************************************\n");
	printf("\t* send [usernum]                       :发送信息                    *\n");
	printf("\t* sendfile [usernum] [filename...]     :发送信息                    *\n");
	printf("\t* getfile [filenum]                    :发送信息                    *\n");
	printf("\t* list/ls                              :查看用户列表                *\n");
	printf("\t* lsfile                               :查看可下载文件列表          *\n");
	printf("\t* lsdir                                :查看当前目录列表           *\n");
	printf("\t* state                                :用户状态 1：在线 0：不在线  *\n");
	printf("\t* clear/cls                            :清屏                        *\n");
	printf("\t* help                                 :帮助                        *\n");
	printf("\t* exit/quit                            :退出                        *\n");
	printf("\t*********************************************************************\e[0m\n");
	return 0;
}

int IPMSG_UserState(char buf[])
{
	int		n;
	if(buf == NULL){
		printf(" \e[31mLess arguments!\n");
		return 0;
	}
	sscanf(buf, "%d", &n);
	if(n == 1){
		user_state = 1;
		//以广播的方式 告诉网内其他用户 用户上线	
		IPMSG_UDP_Send(TO_EVERONE, IPMSG_BR_ENTRY, user_name, strlen(user_name));
	}
	else if(n == 0){
		user_state = 0;
		//以广播的方式 告诉网内其他用户 用户下线	
		IPMSG_UDP_Send(TO_EVERONE, IPMSG_BR_EXIT, user_name, strlen(user_name));
	}
	else
		printf(" \e[31mInvalid argument!\n");
	return 0;
}
//========================================================================
//	语法格式：	int IPMSG_ClearScreen(char buf[])
//	实现功能：	清屏
//	参数：		无
//	返回值：	0 
//========================================================================
int IPMSG_ClearScreen(char buf[])
{
	write(1,"\033[2J",4);   //清屏
	write(1,"\033[0;0H",6); //将光标定位在左上角
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_UserList(char buf[])
//	实现功能：	打印列表
//	参数：		无
//				全局（user_list）
//	返回值：	0 
//========================================================================
int IPMSG_UserList(char buf[])
{
	UserList_Ls(&user_list);
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_FileList(char buf[])
//	实现功能：	打印可下载文件列表
//	参数：		无
//				全局（user_list）
//	返回值：	0 
//========================================================================
int IPMSG_FileList(char buf[])
{
	FileList_List(&file_list);
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_Quit(char buf[])
//	实现功能：	退出
//	参数：		无
//	返回值：	-1 
//========================================================================
int IPMSG_Quit(char buf[])
{
	printf("\e[38mBye!\e[0m\n");
	IPMSG_UDP_Send(TO_EVERONE, IPMSG_BR_EXIT, user_name, strlen(user_name));
	return -1;
}

//========================================================================
//	语法格式：	int IPMSG_DownFile(char buf[])
//	实现功能：	通过TCP协议接收文件
//	参数：		char buf[]
//	返回值：	成功0  遇到错误 -1 
//========================================================================
int IPMSG_DownFile(char *arg)
{
	int					len;				//每次接收 或发送的字符长度
	int					num;				//选择要下载的文件的编号
	int					fd;					//下载文件描述符
	int					s_addr;
	int					sockfd;
	struct sockaddr_in	sockaddr;
	char				str[1024];
	num = *(int *)arg;
	if(FileList_GetUserAddr(&file_list, num, &s_addr) == -1){
		printf(" \e[31mNo such file number(%d) could be download!\e[0m\n", num);
		return 0;
	}
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror(" \e[31m <IPMSG_TCP_GetFile>socket tcp\e[0m");
		return 0;
	}
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(2425);
	sockaddr.sin_addr.s_addr = s_addr;
	if(connect(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1){
		perror(" \e[31m <IPMSG_TCP_GetFile>connect\e[0m\n");
		close(sockfd);
		return 0;
	}
	if((len = sprintf(str, "%d:%ld:%s:%s:%lu:%lx:%x:%x:", 1, time((time_t *)NULL), user_name, host_name, IPMSG_GETFILEDATA, file_list.f_array[num - 1].pkgnum, file_list.f_array[num - 1].num, 0)) < 0){
		printf(" \e[31m<IPMSG_TCP_GetFile>sprintf:maybe input is too larg!\n");
		close(sockfd);
		return 0;
	}
	if(write(sockfd, str, len) == -1){
		perror(" \e[31m<IPMSG_TCP_GetFile>write\e[0m");
		close(sockfd);
		return 0;
	}
	if((fd = open(file_list.f_array[num - 1].name, O_WRONLY|O_CREAT, 777)) == -1){
		printf(" \e[31m<IPMSG_TCP_GetFile>can not create \"%s\"\e[0m\n", file_list.f_array[num - 1].name);
		return 0;
	}
	//int		times = 0;
	long	total = 0;
	char	temp[1024];
	write(1, "\e[s", 3);
	while(1){
		if((len = read(sockfd, temp, sizeof(temp))) < 0){
			perror(" \e[31m <IPMSG_TCP_GetFile>read\e[0m\n");
			close(fd);
			close(sockfd);
			return 0;
		}
		total += len;
		//printf(">>>%d(%d)\n", times++, len);
		if(write(fd, temp, sizeof(temp)) == -1){
			perror(" \e[31mCan not write\e[0m");
			close(fd);
			close(sockfd);
			return 0;
		}
		printf("\e[1;1H[\t>\e[30mFile Down Loading ........................ (\e[32m%3ld\e[0m)", (total*100)/(file_list.f_array[num - 1].size));
		fflush(stdout);
		if(file_list.f_array[num - 1].size == total){
			write(1, "\e[u", 3);
			printf("\n \e[36m(%s)Download Succees!\e[0m\n >", file_list.f_array[num - 1].name);
			break;
		}
	}
	close(fd);
	close(sockfd);
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_TCP_GetFile(char buf[])
//	实现功能：	通过TCP协议接收文件
//	参数：		char buf[]
//	返回值：	成功0  遇到错误 -1 
//========================================================================
int IPMSG_TCP_GetFile(char buf[])
{
	pthread_t	pthread_down;
	int			num;
	if(buf == NULL){
		printf(" \e[31mLess Arguments\e[0m\n");
		return 0;
	}
	if(sscanf(buf, "%d", &num) == EOF){
		printf(" \e[31mLess Arguments\e[0m\n");
		return 0;
	}
	pthread_create(&pthread_down ,NULL,(void *)IPMSG_DownFile, &num);
	usleep(10000);
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_UDP_SendFile(char buf[])
//	实现功能：	将需要发送的文件信息 发送给对方 并将这些信息保存到server_file_list中 给TCP_Server使用
//	参数：		char buf[]
//	返回值：	成功0  遇到错误 -1 
//========================================================================
int IPMSG_UDP_SendFile(char buf[])
{
	int				num;
	int				len;
	char			name[50];
	int				res;
	char			str[1024];
	int				s_addr;
	struct stat		file_info;
	if(buf == NULL){
		printf(" \e[31mLess arguments!\n");
		return 0;
	}
	memset(name, '\0', sizeof(name));
	if(sscanf(buf, " %d %s", &num, name) == EOF){
		printf(" \e[31mLess arguments!\n");
		return 0;
	}
	if(UserList_GetUserAddr(&user_list, num, &s_addr) == -1){
		printf(" \e[31mNo such User\e[0m\n");
		return 0;
	}
	if(stat(name, &file_info) == -1){
		perror(" \e[31m<IPMSG_UDP_SendFile_Str>\e[0m");
		return 0;
	}
	memset(str, '\0', sizeof(str));
	if(S_ISREG (file_info.st_mode))
		res = IPMSG_FILE_REGULAR;
	else if(S_ISDIR(file_info.st_mode))
		res = IPMSG_FILE_DIR;
	if(FileList_Add(&server_file_list, server_file_list.f_count - 1, name, 0, file_info.st_size, file_info.st_mtime, s_addr) == -1){
		return -1;
	}
	FileList_List(&server_file_list);
	if((len = sprintf(str, "%c%d:%s:%lx:%lx:%d:", '\0', 1, name, file_info.st_size, file_info.st_mtime, res)) < 0){
		printf(" \e[31m<IPMSG_UDP_SendFile_Str>sprintf:maybe input is too larg!\n");
		return -1;
	}
	IPMSG_UDP_Send(num, IPMSG_SENDMSG | IPMSG_FILEATTACHOPT | IPMSG_SENDCHECKOPT, str, len);
	return 0;
}

int IPMSG_DirList(char buf[])
{
	DIR		*dp;
	int		i = 0;
	struct dirent *entry;
	dp = opendir("./");
	printf("\t\[35m<Dir List>\e[0m\n ");
	while((entry = readdir(dp)) != NULL){
		printf("\t\t<%2d> %s", ++i, entry->d_name);
		if(i % 4 == 3)
			printf("\n");
	}
	printf("\n");
	fflush(stdout);
	closedir(dp);
	return 0;
}

//========================================================================
//	语法格式：	int IPMSG_GetUserInputOnce(void)
//	实现功能：	获取一次用户输入  解析调用相应函数处理
//	参数：		无
//	返回值：	成功: 0  失败(或调用退出): -1 
//========================================================================
struct CMD_user_input{
	char		cmd[20];
	int	(* fun)(char buf[]);
};
struct CMD_user_input cmd_list2[] = {
							{"sendfile",IPMSG_UDP_SendFile},//发送文件
							{"send",	IPMSG_UDP_Send_Str},	//发送消息
							{"getfile",	IPMSG_TCP_GetFile},		//下载文件
							{"lsdir",	IPMSG_DirList},			//查看当前目录文件
							{"lsfile",	IPMSG_FileList},		//查看可下载文件列表
							{"ls",		IPMSG_UserList},		//查看用户列表
							{"list",	IPMSG_UserList},
							{"clear",	IPMSG_ClearScreen},		//清屏
							{"cls",		IPMSG_ClearScreen},	
							{"state",	IPMSG_UserState},		//设置用户状态 1 在线 0 不在线
							{"help",	IPMSG_Help},			//打印帮助信息
							{"exit",	IPMSG_Quit},			//退出
							{"quit",	IPMSG_Quit}
						};
int IPMSG_GetUserInputOnce(void)
{
	int					i;
	char				buf[1024];
	char				*p_str;
	memset(buf, '\0', sizeof(buf));
	read(0, buf, sizeof(buf));
	buf[strlen(buf) - 1] = '\0';
	printf(" [%s]", buf);
	fflush(stdout);
	for(i = 0; i < sizeof(cmd_list2)/sizeof(struct CMD_user_input); i ++){
		if(strncmp(cmd_list2[i].cmd, buf, strlen(cmd_list2[i].cmd)) == 0){
			if((p_str = strchr(buf, ' ')) != NULL){
				if(cmd_list2[i].fun(p_str + 1) == -1)
					return -1;
			}
			else{
				if(cmd_list2[i].fun(NULL) == -1)
					return -1;
			}
			printf(" \e[34m>");
			fflush(stdout);
			return 0;
		}
	}
	if(buf[0] != '\0'){
		strtok(buf, " \t");
		printf(" \e[31mNoSuchCMD: %s\e[0m\n", buf);
	}
	printf(" \e[34m>");
	fflush(stdout);
	return 0;
}

//========================================================================
//	语法格式：	void *IPMSG_ServerProcess(void *arg)
//	实现功能：	为用户提供下载
//	参数：		TCP套接字
//	返回值：	成功: 0  失败(或调用退出): -1 
//========================================================================
void *IPMSG_ServerProcess(void *arg)
{
	int			num;							//对方请求下载的文件序号（server_file_list）
	int			s_addr;							//可接收文件序号为num的 IP
	int			fd_file;						//对方请求下载的文件描述符
	int			len = 0;
	char		buf[1024] = "";					//缓冲区
	int			connfd = (int)arg;				//TCP套接字
	char		*argv[9];						//用于切割接收到的命令字符串
	int			argc = 0;
	
	if((len = read(connfd, buf, sizeof(buf))) < 0){
		perror(" \e[31mcanot read!\e[0m");
		return NULL;
	}
	//切割接收到的字符串
	argv[argc] = strtok(buf, ":");
	argc ++;
	while((argv[argc] = strtok(NULL, ":")) != NULL ){							//拆分接收到的包
		argc ++;
	}
	if(argc < 8){
		close(connfd);
		return NULL;
	}
	if((atol(argv[4]) & IPMSG_GETFILEDATA) != IPMSG_GETFILEDATA){
		close(connfd);
		return NULL;
	}
	sscanf(argv[6], "%x", &num);
	if(FileList_GetUserAddr(&server_file_list, num, &s_addr) == -1){
		close(connfd);
		return NULL;
	}
	if((fd_file = open(server_file_list.f_array[num - 1].name, O_RDONLY)) == -1){
		perror(" \e[31mOpen\e[0m");
		close(connfd);
		return NULL;
	}
	while(1){
		if((len = read(fd_file, buf, sizeof(buf))) == -1){
			perror(" \e[31mRead\e[0m");
			close(connfd);
			close(fd_file);
			return NULL;
		}
		if(write(connfd, buf, len) == -1){
			perror(" \e[31mWrite\e[0m");
			close(connfd);
			close(fd_file);
			return NULL;
		}
		if(len < sizeof(buf)){
			break;
		}
	}
	//printf("client closed!\n");
	close(fd_file);
	close(connfd);
	return 	NULL;
}

//========================================================================
//	语法格式：	void *IPMSG_TCP_Server(void *arg)
//	实现功能：	TCP 文件传输时使用
//	参数：		
//				全局（user_name, host_name, udp_sockfd）
//	返回值：	NULL
//========================================================================
void *IPMSG_TCP_Server(void *arg)
{
	int					tcp_sockfd;			//TCP socket
	struct sockaddr_in	IPMSG_sockaddr;
	socklen_t			addrlen = sizeof(struct sockaddr);
	//TCP 用于发送 接收文件 （协议:ipv4  端口:2425）(线程)
	if((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("\e[31m <IPMSG_Start>socket tcp\e[0m");
		return NULL;
	}
	memset(&IPMSG_sockaddr, 0, sizeof(IPMSG_sockaddr));
	IPMSG_sockaddr.sin_family = AF_INET;
	IPMSG_sockaddr.sin_port = htons(2425);
	IPMSG_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(tcp_sockfd, (struct sockaddr *)&IPMSG_sockaddr, sizeof(IPMSG_sockaddr)) == -1){
		perror(" \e[31m<IPMSG_Start>bind tcp\0m");		close(udp_sockfd);
		close(tcp_sockfd);
		return NULL;
	}
	if(listen(tcp_sockfd, 5) == -1){
		perror(" \e[31m<IPMSG_Start>listen tcp\0m");
		close(tcp_sockfd);
		return NULL;
	}
	while(1){
		int			connfd;
		pthread_t	thread_id;
		if((connfd = accept(tcp_sockfd, (struct sockaddr*)&IPMSG_sockaddr, &addrlen)) == -1){
			perror(" \e[31mCannot accept\e[0m");
			close(tcp_sockfd);
			return NULL;
		}
		pthread_create(&thread_id ,NULL,(void *)IPMSG_ServerProcess,(void *)connfd);
		pthread_detach(thread_id);
	}
	close(tcp_sockfd);
	return NULL;
}

//========================================================================
//	语法格式：	int IPMSG_Start(char user[], char host[])
//	实现功能：	飞鸽 完成网络通信设置 发送上线广播 监听用户输入，UDP，TCP
//	参数：		char user[]用户名, char host[]主机名
//				全局（user_name, host_name, udp_sockfd）
//	返回值：	成功: 0  失败: -1 
//========================================================================
int IPMSG_Start(char user[], char host[], int pic)
{
	struct sockaddr_in	IPMSG_sockaddr;
	int					optval = 1;													//setsokopt允许UDP广播
	pthread_t			pthread_server;
	//创建用户列表
	if(UserList_Create(&user_list) == -1){
		return -1;
	}
	//创建文件接收列表
	if(FileList_Create(&file_list) == -1){
		UserLiset_FreeList(&user_list);
		return -1;
	}
	//创建可被下载文件列表（TCP server）
	if(FileList_Create(&server_file_list) == -1){
		UserLiset_FreeList(&user_list);
		FileList_FreeList(&file_list);
		return -1;
	}
	memset(&IPMSG_sockaddr, 0, sizeof(IPMSG_sockaddr));
	IPMSG_sockaddr.sin_family = AF_INET;
	IPMSG_sockaddr.sin_port = htons(2425);
	IPMSG_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//UDP 用于发送 接收消息 （协议:ipv4  端口:2425 可以广播）
	if((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		perror("\e[31m <IPMSG_Start>socket udp\e[0m");
		UserLiset_FreeList(&user_list);
		FileList_FreeList(&file_list);
		FileList_FreeList(&server_file_list);
		return -1;
	}
	if(bind(udp_sockfd, (struct sockaddr *)&IPMSG_sockaddr, sizeof(IPMSG_sockaddr)) == -1){
		perror(" \e[31m<IPMSG_Start>bind udp\0m");
		close(udp_sockfd);
		UserLiset_FreeList(&user_list);
		FileList_FreeList(&file_list);
		FileList_FreeList(&server_file_list);
		return -1;
	}
	if(setsockopt(udp_sockfd, SOL_SOCKET, SO_BROADCAST,  &optval, sizeof(optval)) == -1){
		perror(" \e[31m<IPMSG_Start>setsockopt\0m");
		close(udp_sockfd);
		UserLiset_FreeList(&user_list);
		FileList_FreeList(&file_list);
		FileList_FreeList(&server_file_list);
		return -1;
	}
	//启动TCP server线程
	if(pthread_create(&pthread_server, NULL, IPMSG_TCP_Server, NULL) != 0){
		printf(" \e[31m<IPMSG_Start>pthread_create error\e[0m\n");
		close(udp_sockfd);
		UserLiset_FreeList(&user_list);
		FileList_FreeList(&file_list);
		FileList_FreeList(&server_file_list);
		return -1;
	}
	pthread_detach(pthread_server);
	user_pic = pic;
	strncpy(user_name, user, sizeof(user_name));
	strncpy(host_name, host, sizeof(host_name));
	//signal(SININT, _Interrupt);
	//以广播的方式 告诉网内其他用户 用户上线	
	IPMSG_UDP_Send(TO_EVERONE, IPMSG_BR_ENTRY, user_name, strlen(user_name));
	printf(" \e[34m>");
	fflush(stdout);
	while(1)
	{
		fd_set		readfds;
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		FD_SET(udp_sockfd, &readfds);
		if(select(udp_sockfd + 1, &readfds, NULL, NULL, NULL) == -1){				//等待用户输入,等待UDP消息,等待TCP下载请求
			perror(" \e[31m<IPMSG_Start>select\e[0m");
			close(udp_sockfd);
			UserLiset_FreeList(&user_list);
			FileList_FreeList(&file_list);
			FileList_FreeList(&server_file_list);
			return -1;
		}
		//接收解析用户输入命令
		if(FD_ISSET(0, &readfds)){
			if(IPMSG_GetUserInputOnce() == -1){
				close(udp_sockfd);
				UserLiset_FreeList(&user_list);
				FileList_FreeList(&file_list);
				FileList_FreeList(&server_file_list);
				return -1;
			}
		}
		//接收UDP消息并解析
		if(FD_ISSET(udp_sockfd, &readfds)){
			if(IPMSG_ReceiveUdpMSGOnce() == -1){
				close(udp_sockfd);
				UserLiset_FreeList(&user_list);
				FileList_FreeList(&file_list);
				FileList_FreeList(&server_file_list);
				return -1;
			}
		}
	}
}


