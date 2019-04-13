/* ************************************************************************
 *       Filename:  FileList.h
 *    Description:  
 *        Version:  1.0
 *        Created:  2012年06月25日 09时41分34秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/
//文件列表(实现方式与UserList.c方式相同)
//文件信息结构体
#ifndef	FILELIST_H
#define FILELIST_H

struct file_array
{
	char			name[50];			//文件名
	int 			num;				//文件序号
	long			pkgnum;				//包编号
	long			size;				//文件大小
	long			ltime;				//最后修改时间
	int				s_addr;				//发送者IP(网络字节序)
	struct filelist *next;
};

struct file_info{
	struct file_array	*f_array;		//指向用户列表（动态数组 malloc realloc 实现）
	int					f_volume;		//用户列表容量（file_list_INCREASE 的倍数）
	int					f_count;		//实际用户数量 (该值 <= 用户列表容量)
};

#define		FILE_LIST_INCREASE	5		//当需要增添新用户 而实际用户数量已经等于用户列表容量 此时用户列表容量会自动增加
										//file_list_INCREASE个											
//========================================================================
//	语法格式：	int FileList_VolumeIncrease(struct file_info *file_list)
//	实现功能：	为文件列表申请空间 或为列表扩容一次
//				file_list 为NULL时 申请新空间   否则进行一次扩容
//	参数：		struct file_info *file_list
//	返回值：	成功: 0  失败: -1 
//========================================================================
int FileList_VolumeIncrease(struct file_info *file_list);

//========================================================================
//	语法格式：	int	FileList_Create(struct file_info *file_list)
//	实现功能：	建立一个文件列表
//	参数：		struct file_info *file_list
//	返回值：	成功: 0  失败: -1 
//========================================================================
int	FileList_Create(struct file_info *file_list);

//========================================================================
//	语法格式：	void FileList_List(struct file_info *file_list)
//	实现功能：	打印文件列表
//	参数：		struct file_info *file_list
//	返回值：	无
//========================================================================
void FileList_List(struct file_info *file_list);

//========================================================================
//	语法格式：	int FileList_Add(struct file_info *file_list, int num, char file_name[], long pkgnum, long size, long ltime, int s_addr)
//	实现功能：	向文件列表中添加信息
//	参数：		struct file_info *file_list, char name[], char host[], int	s_addr(IP地址32位网络字节)
//	返回值：	成功: 0   失败：-1
//========================================================================
int FileList_Add(struct file_info *file_list, int num, char file_name[], long pkgnum, long size, long ltime, int s_addr);

//========================================================================
//	语法格式：	int FileList_GetUserAddr(struct file_info *file_list, int num, int *s_addr)
//	实现功能：	从file_list中查找第num个文件发送者的网络IP(网络字序) 保存到s_addr指向的空间中
//	参数：		struct file_info *file_list, int num, int *s_addr
//	返回值：	成功: 0  失败: -1 
//========================================================================
int FileList_GetUserAddr(struct file_info *file_list, int num, int *s_addr);

//========================================================================
//	语法格式：	int FileList_Find(struct file_info *file_list, int s_addr)
//	实现功能：	从列表中查找的用户 并返回其序号（>=1）
//	参数：		struct file_info *file_list, int s_addr
//	返回值：	找到：序号（>=1） 没有找到：-1
//========================================================================
int FileList_Find(struct file_info *file_list, int s_addr);

//========================================================================
//	语法格式：	void FileList_FreeList(struct file_info *file_list)
//	实现功能：	释放file_list列表
//	参数：		struct file_info *file_list
//	返回值：	无
//========================================================================
void FileList_FreeList(struct file_info *file_list);

#endif
