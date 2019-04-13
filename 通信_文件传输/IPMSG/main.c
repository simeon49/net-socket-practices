/* ************************************************************************
 *       Filename:  main.c
 *    Description:  通过调用IPMSG来实现飞鸽传书的功能
 *        Version:  1.0
 *        Created:  2012年06月20日 13时46分43秒
 *       Compiler:  gcc
 *         Author:  Ebenezer
 * ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "IPMSG.h"

//========================================================================
//	语法格式：	int main(int argc, char *argv[])
//	实现功能：	启动IPMSG飞鸽
//	参数：		int argc, char *argv[](本机用户名)
//	返回值：	成功: 0  失败: -1 
//========================================================================
int main(int argc, char *argv[])
{
	char	user[20];
	char	host[20];
	int		pic_num;
	//获取本机用户名 主机名
	if(argc != 4){
		printf(" \e[31m[Example]:\e[35m ./run User Host 74\e[0m\n");
		printf(" \e[31m<main>Need a UserName and a HostName[20] a PicNum\e[0m\n");
		return 0;
	}
	strncpy(user, argv[1], sizeof(user));
	user[sizeof(user)-1] = '\0';
	strncpy(host, argv[2], sizeof(host));
	host[sizeof(host)-1] = '\0';
	pic_num = atoi(argv[3]);
	printf(" \e[35mWhere come!\t(Mr./Miss):%s\t(Host): %s\e[0m\n", user, host);
	//启动飞鸽
	if(IPMSG_Start(user, host, pic_num) == -1)
		return -1;
	return 0;
}




