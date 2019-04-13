/* ************************************************************************
 *       Filename:  user_interface.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2010��03��08�� 10ʱ28��50��
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/
#include "myinclude.h"
#include "user_manager.h"
#include "file_manager.h"
#include "user_interface.h"
#include "communication.h"
//�����˵�
char *help= "*********************************************************************************\n"\
			"* send/say [username]		        :    ������Ϣ		        *\n"\
			"* sendfile [username] [filename...]  	:    �����ļ�		        *\n"\
			"* getfile  [filenum]			:    �����ļ�		        *\n"\
			"* list/ls				:    ��ӡ�û��б�		*\n"\
			"* list/ls file				:    ��ӡ�����ļ��б�		*\n"\
			"* clear/cls				:    ����			*\n"\
			"* help					��   ����			*\n"\
			"* exit/quit				:    �Ƴ�			*\n"\
			"**********************************************************************************\n"
			;
//����ָ��
typedef void (*FUN)(int argc, char *argv[]);

//����ṹ��
typedef struct cmd
{
	char *name;		//��������
	FUN fun;		//�������
}CMD;


//��������
//send [user] [msg]
//sendfile [user] [file1] [file2] ...
void send_fun(int argc, char *argv[])
{
	struct sockaddr_in addr = {AF_INET};
	int s_addr = 0;
	char name[20] = "";
	char buf[100] = "";	
	
	char sendbuf[200]="";
	char temp[100]="";
	int templen = 0;
	long t = time((time_t*)NULL);
	int len = sprintf(sendbuf, "1:%ld:%s:%s", t, user(), host());
	
	if(argc < 2)	//����С��2�����������û���
	{
		list();
		IPMSG_OUT_MSG_COLOR(
		printf("please input user name:");
		)
		fgets(name, sizeof(name), stdin);
		name[strlen(name)-1] = 0;
		argv[1] = name; 
	}
	
	s_addr = get_addr_by_name(argv[1]);
	if(s_addr == 0)
	{
		printf("error: no this user:\"%s\"!\n", argv[1]);
		return ;
	}
	if(argc < 3)	//����С��2������������Ϣ���ļ���
	{
		if(strcmp(argv[0], "sendfile")==0)
		{
			IPMSG_OUT_MSG_COLOR(			
			printf("please input filename:");
			)
		}
		else
		{
			IPMSG_OUT_MSG_COLOR(	
			printf("please input message:");
			)
		}
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf)-1] = 0;
		argv[2] = buf;
		
		//����Ƿ����ļ������и�����Ķ���ļ���
		if( argv[0][4]=='f' )	
		{
			int i = 2;
			argv[i] = buf;
			while((argv[i]=strtok(argv[i], " \t"))!=NULL) i++;
		}
	}	
	
	if(strcmp(argv[0], "sendfile")==0)
	{
		int i = 2;
		char *fileopt = NULL;
		templen = sprintf(temp, ":%ld:%c",IPMSG_SENDMSG|IPMSG_SENDCHECKOPT|IPMSG_FILEATTACHOPT,0);
		fileopt = temp+templen;
		//��Ӷ���ļ�����
		while(argv[i]!=NULL)
		{
			len = getfileopt(fileopt, argv[i], t, i-2);
			templen += len;
			fileopt += len;
			i++;
		}
	}
	else
	{
		templen = sprintf(temp, ":%ld:%s", IPMSG_SENDMSG|IPMSG_SENDCHECKOPT, argv[2]);
	}
	
	memcpy(sendbuf+len, temp, templen);
	
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = s_addr;
	msg_send(sendbuf,len+templen, addr);
}

//�����ļ�
//�����ʽ: getfile num
// num :	�ļ��������е���ţ����á�ls file������鿴
void getfile_fun(int argc, char *argv[])
{
	int id = 0;
	if( argc!=2 )
	{
		IPMSG_OUT_MSG_COLOR(
		printf("error cmd param\n");
		printf("command: getfile num\n");
		)
		return ;
	}
	if((id=atoi(argv[1]))<0)
	{
		IPMSG_OUT_MSG_COLOR(
		printf("No such file!\n");
		)
		return ;
	}
	recvfile(id);
}

//��ӡ�û����ļ��б�
//ls	  : ��ӡ�û��б�
//ls file : ��ӡ�����ļ��б�
void list_fun(int argc, char *argv[])
{
	if( argv[1]==NULL )
		list();				//�û��б�
	else if(strcmp("file", argv[1])==0)
		file_list();		//�ļ��б�
	else 
		printf("command not find!\n");
}

// ��������
void help_fun(int argc, char *argv[])
{
	//��ӡ�����˵�
	IPMSG_OUT_MSG_COLOR(
	printf("%s", help);	
	)
}

//�˳�����
void exit_fun(int argc, char *argv[])
{
	ipmsg_exit();
//	free_link();
	exit(1);
}

//��������
void clear_fun(int argc, char *argv[])
{
	write(1, "\033[2J\033[0;0H", 10);
}

//�������飺�������� ������ �� ��������
CMD cmdlist[]={	
				{"send", send_fun},
				{"say", send_fun},					
				{"sendfile", send_fun},		
				{"getfile", getfile_fun},
				{"list", list_fun},		
				{"ls", list_fun},	
				{"help", help_fun},
				{"exit", exit_fun},
				{"quit", exit_fun},
				{"clear", clear_fun},			
				{"cls", clear_fun}	
			};

//��������������
int exec_cmd(char *cmd)
{
	char *argv[10] = {NULL};
	int argc = 0;
	int i = 0;
	if(strlen(cmd)==0)
		return 0;
		
	argv[0] = cmd;
	while((argv[argc]=strtok(argv[argc], " \t"))!=NULL) argc++;
	
	/*add by wenhao*/
	if(argc == 0){
		return -1;
	}
	/*end by wenhao*/

	for (i=0;i<sizeof(cmdlist)/sizeof(CMD);i++)
	{
		//��������
		if (strcmp(cmdlist[i].name, argv[0])==0)
		{
			//ִ������
			cmdlist[i].fun(argc, argv);
			return 0;
		}
	}	
	return -1;
}			

void *user_interface(void *arg)
{
	write(1, "\033[32m", 5);
	while(1)
	{
		char buf[100]="";
		write(1,"\rIPMSG:",7);
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf)-1]=0;
		if(exec_cmd(buf) < 0)
		{
			IPMSG_OUT_MSG_COLOR(
			printf("command not find!\n");
			)
		}
	}
	return NULL;
}

