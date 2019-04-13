/* ************************************************************************
 *       Filename:  UserList.c		(用户列表  创建、添加、删除、查找...)
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月22日 13时55分40秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "UserList.h"

//========================================================================
//	语法格式：	int UserLiset_VolumeIncrease(struct user_info *user_list)
//	实现功能：	为新用户列表申请空间 或为列表扩容一次
//				user_list 为NULL时 申请新空间   否则进行一次扩容
//	参数：		struct user_info *user_list
//	返回值：	成功: 0  失败: -1 
//========================================================================
int UserLiset_VolumeIncrease(struct user_info *user_list)
{
	if(user_list->u_array == NULL){
		if((user_list->u_array = (struct user_array *)malloc(USER_LIST_INCREASE * sizeof(struct user_array))) == NULL){
			user_list->u_volume = 0;
			perror(" \e[31m<UserLiset_VolumeIncrease>malloc \e[0m");
			return -1;
		}
		user_list->u_volume = USER_LIST_INCREASE;
	}
	else{
		struct user_array	*p_realloc;
		if((p_realloc = (struct user_array *)realloc(user_list->u_array, (user_list->u_volume + USER_LIST_INCREASE)* sizeof(struct user_array))) == NULL){
			printf(" \e[31m<UserLiset_VolumeIncrease>realloc \e[0m\n");
			free(user_list->u_array);
			user_list->u_array = NULL;
			user_list->u_count = 0;
			user_list->u_volume = 0;
			return -1;
		}
		user_list->u_array = p_realloc;
		user_list->u_volume += USER_LIST_INCREASE;
	}
	return 0;
}

//========================================================================
//	语法格式：	int	UserList_Create(struct user_info *user_list)
//	实现功能：	建立一个用户列表
//	参数：		struct user_info *user_list
//	返回值：	成功: 0  失败: -1 
//========================================================================
int	UserList_Create(struct user_info *user_list)
{
	user_list->u_array = NULL;
	user_list->u_count = 0;
	return (UserLiset_VolumeIncrease(user_list));
}

//========================================================================
//	语法格式：	int UserList_Add(struct user_info *user_list, char name[], char host[], int	s_addr)
//	实现功能：	向用户列表中添加用户
//	参数：		struct user_info *user_list, char name[], char host[], int	s_addr(IP地址32位网络字节)
//	返回值：	成功: 0   失败：-1
//========================================================================
int UserList_Add(struct user_info *user_list, char name[], char host[], int	s_addr)
{
	if(user_list->u_count == user_list->u_volume){										//当user_list容量不足时 进行扩容
		if(UserLiset_VolumeIncrease(user_list) == -1)
			return -1;
	}
	memset(&user_list->u_array[user_list->u_count], 0, sizeof(struct user_array));
	strncpy(user_list->u_array[user_list->u_count].name, name, sizeof(user_list->u_array[user_list->u_count].name) - 1);
	strncpy(user_list->u_array[user_list->u_count].host, host, sizeof(user_list->u_array[user_list->u_count].host) - 1);
	user_list->u_array[user_list->u_count].s_addr = s_addr;
	user_list->u_count ++;
	//UserList_Ls(user_list);
	return 0;
}

//========================================================================
//	语法格式：	int UserList_Del(struct user_info *user_list, int who)
//	实现功能：	向用户列表删除who(>=1)用户 并将表内最后一个用户信息copy到who所在的空间
//	参数：		struct user_info *user_list, int who（所有用户的序号从 1 开始）
//	返回值：	成功: 0  失败: -1 
//========================================================================
int UserList_Del(struct user_info *user_list, int who)
{
	if(who <= 0 || who > user_list->u_count)
		return -1;
	memset(&user_list->u_array[who - 1], 0, sizeof(struct user_array));
	strncpy((user_list->u_array)[who - 1].name, (user_list->u_array)[user_list->u_count - 1].name, sizeof((user_list->u_array)[user_list->u_count - 1].name));
	strncpy((user_list->u_array)[who - 1].host, (user_list->u_array)[user_list->u_count - 1].host, sizeof((user_list->u_array)[user_list->u_count - 1].host));
	user_list->u_array[who - 1].s_addr = (user_list->u_array)[user_list->u_count - 1].s_addr;
	user_list->u_count --;
	return 0;
}

//========================================================================
//	语法格式：	int UserList_UserInfoRewrite(struct user_info *user_list, int who, char name[], char host[], int s_addr)
//	实现功能：	修改who用户的信息
//	参数：		struct user_info *user_list, int who, char name[], char host[], int	s_addr(IP地址32位网络字节)
//	返回值：	成功: 0   失败：-1
//========================================================================
int UserList_UserInfoRewrite(struct user_info *user_list, int who, char name[], char host[], int s_addr)
{
	if(who <= 0 || who > user_list->u_count)
		return -1;
	memset(&user_list->u_array[who - 1], 0, sizeof(struct user_array));
	strncpy(user_list->u_array[who - 1].name, name, sizeof(user_list->u_array[who - 1].name) - 1);
	strncpy(user_list->u_array[who - 1].host, host, sizeof(user_list->u_array[who - 1].host) - 1);
	user_list->u_array[who - 1].s_addr = s_addr;
	return 0;
}

//========================================================================
//	语法格式：	void UserList_Ls(struct user_info *user_list)
//	实现功能：	打印用户列表
//	参数：		struct user_info *user_list
//	返回值：	无
//========================================================================
void UserList_Ls(struct user_info *user_list)
{
	int		i;
	char	ip[INET_ADDRSTRLEN];
	printf("\n");
	printf("  \e[36mN0.\t  Name\t\t  Host\t\t  IP\e[0m\n");
	for( i = 0; i < user_list->u_count; i++){
		inet_ntop(AF_INET, &(user_list->u_array[i].s_addr), ip, sizeof(ip));
		if(i%2 == 0)
			printf("   %d\t%s\t\t%s\t\t%s\n", i+1, user_list->u_array[i].name, user_list->u_array[i].host, ip);
		else
			printf("   \e[37m%d\t%s\t\t%s\t\t%s\e[0m\n", i+1, user_list->u_array[i].name, user_list->u_array[i].host, ip);
	}
	printf("\n");
}

//========================================================================
//	语法格式：	int UserList_Find(struct user_info *user_list, int mode, void *args)
//	实现功能：	从列表中查找的用户 并返回其序号（>=1）
//	参数：		struct user_info *user_list, int mode(查找模式), void *args（查找的关键字）
//	返回值：	找到：序号（>=1） 没有找到：-1
//========================================================================
//		USERLIST_F_NAME		0		//按用户名查找
//		USERLIST_F_HOST		1		//按用户主机名查找
//		USERLIST_F_ADDR		2		//按用户ip地址查找
int UserList_Find(struct user_info *user_list, int mode, void *args)
{
	int 	i;
	switch(mode){
		case USERLIST_F_NAME:
			for(i = 0; i < user_list->u_count; i ++){
				if(strcmp(user_list->u_array[i].name, (char *)args) == 0){
					return (i + 1);
				}
			}
			break;
		case USERLIST_F_HOST:
			for(i = 0; i < user_list->u_count; i ++){
				if(strcmp(user_list->u_array[i].host, (char *)args) == 0){
					return (i + 1);
				}
			}
			break;
		case USERLIST_F_ADDR:
			for(i = 0; i < user_list->u_count; i ++){
				if(user_list->u_array[i].s_addr == *(int *)args){
					return (i + 1);
				}
			}
			break;
		default:
			break;
	}
	return -1;
}

//========================================================================
//	语法格式：	int UserList_GetUserAddr(struct user_info *user_list, int who, int *s_addr)
//	实现功能：	从user_list中查找第who个用户的网络IP(网络字序) 保存到s_addr指向的空间中
//	参数：		struct user_info *user_list, int who, int *s_addr
//	返回值：	成功: 0  失败: -1 
//========================================================================
int UserList_GetUserAddr(struct user_info *user_list, int who, int *s_addr)
{
	if(who == TO_EVERONE){
		inet_pton(AF_INET, "192.168.220.255", s_addr);							//广播地址
		return 0;
	}
	else if(who > 0 && who <= user_list->u_count){
		*s_addr = user_list->u_array[who -1].s_addr;
		return 0;
	}
	else{
		return -1;
	}
}

//========================================================================
//	语法格式：	void UserLiset_FreeList(struct user_info *user_list)
//	实现功能：	释放user_list列表
//	参数：		struct user_info *user_list
//	返回值：	无
//========================================================================
void UserLiset_FreeList(struct user_info *user_list)
{
	if(user_list->u_array != NULL){
		free(user_list->u_array);
		user_list->u_array = NULL;
		user_list->u_volume = 0;
		user_list->u_count = 0;
	}
}
/*Test
int main(int argc, char *argv[])
{
	int		who;
	int		times = 7;
	struct user_info	user_list;
	printf(" Create\n");
	UserList_Create(&user_list);
	printf(" Add\n");
	while(times --){
		char	name[500];
		char	host[500];
		int		s_addr;
		printf(" name\thost\ts_addr\n");
		printf(" >");
		fflush(stdout);
		scanf("%s %s %d", name, host, &s_addr);
		if((who = UserList_Find(&user_list, USERLIST_F_ADDR, &s_addr)) != -1){
			printf(" R\n");
			UserList_UserInfoRewrite(&user_list, who, name, host, s_addr);
		}
		else{
			printf(" A\n");
			UserList_Add(&user_list, name, host, s_addr);
		}
	}
	UserList_Ls(&user_list);
	printf(" Del No.2\n");
	UserList_Del(&user_list, 2);
	UserList_Ls(&user_list);
	printf(" free list\n");
	UserLiset_FreeList(&user_list);
	return 0;
}*/
