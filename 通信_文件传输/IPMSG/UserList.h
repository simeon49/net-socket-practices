/* ************************************************************************
 *       Filename:  UserList.h (用户列表  创建、添加、删除、查找...)
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月22日 13时56分04秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/
 
 /*
//user_list指向(mallocc出来的)用户列表头 该列表随着网络上人数 动态增加(realloc)
//每次增加的数量为USER_LIST_INCREASE, user_list_volume记录 p_user_list指向的内存的
//实际大小user_list_count 记录当前实际在线人数的个数
*/

#ifndef	USERLIST_H
#define	USERLIST_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>

struct user_array{
	char				name[20];		//用户名
	char				host[20];		//主机名
	int					s_addr;			//IP地址（32位网路字节序）
	//struct user_info	*next;
};

struct user_info{
	struct user_array	*u_array;		//指向用户列表（动态数组 malloc realloc 实现）
	int					u_volume;		//用户列表容量（USER_LIST_INCREASE 的倍数）
	int					u_count;		//实际用户数量 (该值 <= 用户列表容量)
};

#define		USER_LIST_INCREASE	5		//当需要增添新用户 而实际用户数量已经等于用户列表容量 此时用户列表容量会自动增加
										//USER_LIST_INCREASE个											

//========================================================================
//	语法格式：	int	UserList_Create(struct user_info *user_list)
//	实现功能：	建立一个用户列表
//	参数：		struct user_info *user_list
//	返回值：	成功: 0  失败: -1 
//========================================================================
int	UserList_Create(struct user_info *user_list);

//========================================================================
//	语法格式：	int UserList_Add(struct user_info *user_list, char name[], char host[], int	s_addr)
//	实现功能：	向用户列表中添加用户
//	参数：		struct user_info *user_list, char name[], char host[], int	s_addr(IP地址32位网络字节)
//	返回值：	成功: 0   失败：-1
//========================================================================
int UserList_Add(struct user_info *user_list, char name[], char host[], int	s_addr);

//========================================================================
//	语法格式：	int UserList_Del(struct user_info *user_list, int who)
//	实现功能：	向用户列表删除who(>=1)用户 并将表内最后一个用户信息copy到who所在的空间
//	参数：		struct user_info *user_list, int who（所有用户的序号从 1 开始）
//	返回值：	成功: 0  失败: -1 
//========================================================================
int UserList_Del(struct user_info *user_list, int who);

//========================================================================
//	语法格式：	int UserList_UserInfoRewrite(struct user_info *user_list, int who, char name[], char host[], int s_addr)
//	实现功能：	修改who用户的信息
//	参数：		struct user_info *user_list, int who, char name[], char host[], int	s_addr(IP地址32位网络字节)
//	返回值：	成功: 0   失败：-1
//========================================================================
int UserList_UserInfoRewrite(struct user_info *user_list, int who, char name[], char host[], int s_addr);

//========================================================================
//	语法格式：	void UserList_Ls(struct user_info *user_list)
//	实现功能：	打印用户列表
//	参数：		struct user_info *user_list
//	返回值：	无
//========================================================================
void UserList_Ls(struct user_info *user_list);

//========================================================================
//	语法格式：	int UserList_Find(struct user_info *user_list, int mode, void *args)
//	实现功能：	从列表中查找的用户 并返回其序号（>=1）
//	参数：		struct user_info *user_list, int mode(查找模式), void *args（查找的关键字）
//	返回值：	找到：序号（>=1） 没有找到：-1
//========================================================================
#define			USERLIST_F_NAME		0		//按用户名查找
#define			USERLIST_F_HOST		1		//按用户主机名查找
#define			USERLIST_F_ADDR		2		//按用户ip地址查找
int UserList_Find(struct user_info *user_list, int mode, void *args);

//========================================================================
//	语法格式：	int UserList_GetUserAddr(struct user_info *user_list, int who, int *s_addr)
//	实现功能：	从user_list中查找第who个用户的网络IP(网络字序) 保存到s_addr指向的空间中
//	参数：		struct user_info *user_list, int who, int *s_addr
//	返回值：	成功: 0  失败: -1 
//========================================================================
#define		TO_EVERONE			0		//广播告诉网内所有人
int UserList_GetUserAddr(struct user_info *user_list, int who, int *s_addr);

//========================================================================
//	语法格式：	void UserLiset_FreeList(struct user_info *user_list)
//	实现功能：	释放user_list列表
//	参数：		struct user_info *user_list
//	返回值：	无
//========================================================================
void UserLiset_FreeList(struct user_info *user_list);

#endif
