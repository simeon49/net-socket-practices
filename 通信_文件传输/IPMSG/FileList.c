/* ************************************************************************
 *       Filename:  FileList.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月25日 09时41分18秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "FileList.h"
  								
//========================================================================
//	语法格式：	int FileList_VolumeIncrease(struct file_info *file_list)
//	实现功能：	为文件列表申请空间 或为列表扩容一次
//				file_list 为NULL时 申请新空间   否则进行一次扩容
//	参数：		struct file_info *file_list
//	返回值：	成功: 0  失败: -1 
//========================================================================
int FileList_VolumeIncrease(struct file_info *file_list)
{
	if(file_list->f_array == NULL){
		if((file_list->f_array = (struct file_array *)malloc(FILE_LIST_INCREASE * sizeof(struct file_array))) == NULL){
			file_list->f_volume = 0;
			perror(" \e[31m<FileList_VolumeIncrease>malloc \e[0m");
			return -1;
		}
		file_list->f_volume = FILE_LIST_INCREASE;
	}
	else{
		struct file_array	*p_realloc;
		if((p_realloc = (struct file_array *)realloc(file_list->f_array, (file_list->f_volume + FILE_LIST_INCREASE)* sizeof(struct file_array))) == NULL){
			printf(" \e[31m<FileList_VolumeIncrease>realloc \e[0m\n");
			free(file_list->f_array);
			file_list->f_array = NULL;
			file_list->f_count = 0;
			file_list->f_volume = 0;
			return -1;
		}
		file_list->f_array = p_realloc;
		file_list->f_volume += FILE_LIST_INCREASE;
	}
	return 0;
}

//========================================================================
//	语法格式：	int	FileList_Create(struct file_info *file_list)
//	实现功能：	建立一个文件列表
//	参数：		struct file_info *file_list
//	返回值：	成功: 0  失败: -1 
//========================================================================
int	FileList_Create(struct file_info *file_list)
{
	file_list->f_array = NULL;
	file_list->f_count = 0;
	return (FileList_VolumeIncrease(file_list));
}

//========================================================================
//	语法格式：	void FileList_List(struct file_info *file_list)
//	实现功能：	打印文件列表
//	参数：		struct file_info *file_list
//	返回值：	无
//========================================================================
void FileList_List(struct file_info *file_list)
{
	int		i;
	char	ip[INET_ADDRSTRLEN];
	printf("\n");
	printf("  \e[36mNo.\t  FileName\t\t  Size\t\t  SenderIP\e[0m\n");
	for( i = 0; i < file_list->f_count; i++){
		inet_ntop(AF_INET, &(file_list->f_array[i].s_addr), ip, sizeof(ip));
		if(i%2 == 0)
			printf("   %d\t%s\t\t%.3f(KB)\t\t%s\n", i + 1, file_list->f_array[i].name, file_list->f_array[i].size/1024.0, ip);
		else
			printf("   \e[37m%d\t%s\t\t%.3f(KB)\t\t%s\e[0m\n", i + 1, file_list->f_array[i].name, file_list->f_array[i].size/1024.0, ip);
	}
	printf("\n");
}

//========================================================================
//	语法格式：	int FileList_Add(struct file_info *file_list, int num, char file_name[], long pkgnum, long size, long ltime, int s_addr)
//	实现功能：	向文件列表中添加信息
//	参数：		struct file_info *file_list, char name[], char host[], int	s_addr(IP地址32位网络字节)
//	返回值：	成功: 0   失败：-1
//========================================================================
int FileList_Add(struct file_info *file_list, int num, char file_name[], long pkgnum, long size, long ltime, int s_addr)
{
	if(file_list->f_count == file_list->f_volume){										//当file_list容量不足时 进行扩容
		if(FileList_VolumeIncrease(file_list) == -1)
			return -1;
	}
	memset(&file_list->f_array[file_list->f_count], 0, sizeof(struct file_array));
	strncpy(file_list->f_array[file_list->f_count].name, file_name, sizeof(file_list->f_array[file_list->f_count].name) - 1);
	file_list->f_array[file_list->f_count].num = num;
	file_list->f_array[file_list->f_count].pkgnum = pkgnum;
	file_list->f_array[file_list->f_count].size = size;
	file_list->f_array[file_list->f_count].ltime = ltime;
	file_list->f_array[file_list->f_count].s_addr = s_addr;
	file_list->f_count ++;
	//FileList_Ls(file_list);
	return 0;
}

//========================================================================
//	语法格式：	int FileList_GetUserAddr(struct file_info *file_list, int num, int *s_addr)
//	实现功能：	从file_list中查找第num个文件发送者的网络IP(网络字序) 保存到s_addr指向的空间中
//	参数：		struct file_info *file_list, int num, int *s_addr
//	返回值：	成功: 0  失败: -1 
//========================================================================
int FileList_GetUserAddr(struct file_info *file_list, int num, int *s_addr)
{
	if(num > 0 && num <= file_list->f_count){
		*s_addr = file_list->f_array[num -1].s_addr;
		return 0;
	}
	else{
		return -1;
	}
}

//========================================================================
//	语法格式：	int FileList_Find(struct file_info *file_list, int s_addr)
//	实现功能：	从列表中查找的用户 并返回其序号（>=1）
//	参数：		struct file_info *file_list, int s_addr
//	返回值：	找到：序号（>=1） 没有找到：-1
//========================================================================
int FileList_Find(struct file_info *file_list, int s_addr)
{
	int 		i;
	for(i = 0; i < file_list->f_count; i ++){
		if(file_list->f_array[i].s_addr == s_addr){
			return (i + 1);
		}
	}
	return -1;
}

//========================================================================
//	语法格式：	void FileList_FreeList(struct file_info *file_list)
//	实现功能：	释放file_list列表
//	参数：		struct file_info *file_list
//	返回值：	无
//========================================================================
void FileList_FreeList(struct file_info *file_list)
{
	if(file_list->f_array != NULL){
		free(file_list->f_array);
		file_list->f_array = NULL;
		file_list->f_volume = 0;
		file_list->f_count = 0;
	}
}
