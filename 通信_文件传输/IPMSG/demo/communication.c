/* ************************************************************************
 *       Filename:  communicatioh.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2010��03��08�� 10ʱ28��50��
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/
#include "myinclude.h"
#include "communication.h"
#include "user_manager.h"
#include "file_manager.h"

static int udpfd = 0;
static int tcpfd = 0;
static char user_name[20] = "";
static char host_name[30] = "";

//����UDP �� TCP_Server �׽ӿ�
void create_server()
{
	int broadcast=1;
	struct sockaddr_in addr = {AF_INET};
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//Create TCP socket  for SendFile Server
	tcpfd = socket(AF_INET,SOCK_STREAM,0);
	if(tcpfd < 0)
	{
		perror("Socket TCP");
		exit(-1);
	}
	
	if(bind(tcpfd, (struct sockaddr*)&addr, sizeof(addr))<0)
	{
		perror("Bind UDP");
		exit(1);
	}
	listen(tcpfd, 10);
	//Create UDP socket for commucation
	udpfd = socket(AF_INET,SOCK_DGRAM,0);
	if(udpfd < 0)
	{
		perror("Socket UDP");
		exit(-1);
	}
	
	if(bind(udpfd, (struct sockaddr*)&addr, sizeof(addr))<0)
	{
		perror("Bind UDP");
		exit(1);
	}
	setsockopt(udpfd,SOL_SOCKET,SO_BROADCAST,&broadcast, sizeof(int));
}

//���߹㲥
void broad_cast_online_info(void)
{
	char buf[100]="";
	struct sockaddr_in addr = {AF_INET};
	int t = time((time_t *)NULL);
	int len = sprintf(buf,"1:%d:%s:%s:%ld:%s", \
					  t,user_name,host_name,IPMSG_BR_ENTRY,user_name);
	addr.sin_port = htons(PORT);
//	addr.sin_addr.s_addr=inet_addr("255.255.255.255");	
	addr.sin_addr.s_addr = htonl(-1);	//modified by wangyanjun in 10/7/9
	sendto(udpfd, buf, len, 0, (struct sockaddr*)&addr,sizeof(addr));	
}

int tcp_fd(void)
{
	return tcpfd;
}

int udp_fd(void)
{
	return udpfd;
}

char *user(void)
{
	return user_name;
}

char *host(void)
{
	return host_name;
}

//���߹㲥
void ipmsg_exit(void)
{
	char buf[100]="";
	struct sockaddr_in addr = {AF_INET};
	int t = time((time_t *)NULL);
	int len = sprintf(buf,"1:%d:%s:%s:%ld:%s", \
					  t,user_name,host_name,IPMSG_BR_EXIT,user_name);
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr=inet_addr("255.255.255.255");		
	sendto(udpfd, buf, len, 0, (struct sockaddr*)&addr,sizeof(addr));	
}

//���߲���ʼ��ϵͳ
void online(char *user, char *host)
{
	strcpy(user_name,user);
	strcpy(host_name,host);
	create_server();
	broad_cast_online_info();
}

//������Ϣ
void msg_send(char *msg, int len,  struct sockaddr_in addr)
{
	sendto(udpfd, msg, len, 0, (struct sockaddr*)&addr, sizeof(addr));
}

//�����ļ�(����Ϊ�����ļ��б��е����)
int recvfile(int id)
{
	int fd = 0;
	char buf[2048]="";
	FILE *fp = NULL;
	unsigned long len = 0;
	struct sockaddr_in addr = {AF_INET};
	int s_addr = 0;
	IPMSG_FILE *p = find_file(id);		//�Ƿ���ڸ��ļ�
	if( p==NULL	)
	{
		IPMSG_OUT_MSG_COLOR(
		printf("no such file id\n");
		)
		return -1;
	}
	
	s_addr = get_addr_by_name(p->user);	//���ݷ�����������ȡ�������ַ
	if( s_addr == 0 )
	{
		IPMSG_OUT_MSG_COLOR(
		printf("recv file error: user is not online!\n");
		)
		del_file(p, RECVFILE);
		return -1;
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);	//������ʱTCP client���������ļ�
	if( fd < 0 )
	{
		IPMSG_OUT_MSG_COLOR(
		printf("recv file error: creat socket error!\n");
		)
		return -1;
	}
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = s_addr;
	if(connect(fd, (struct sockaddr*)&addr, sizeof(addr))!=0)
	{
		perror("recvfile connect");
		return -1;
	}
	len = sprintf(buf, "1:%ld:%s:%s:%ld:%lx:%d:0", time((time_t*)NULL),\
			user(), host(), IPMSG_GETFILEDATA, p->pkgnum, p->num);
	send(fd, buf, len, 0);		//����IPMSG_GETFILEDATA
	fp = fopen(p->name, "w");
	if( fp==NULL )
	{
		perror("savefile");
		return -1;
	}
	len = 0;
	do				//�����ļ�
	{
		int rlen = recv(fd, buf, sizeof(buf), 0);
		len += rlen;
		IPMSG_OUT_MSG_COLOR(
		printf("\rrecvlen=%d%%",  (int)((100*len)/p->size));

		)
		fflush(stdout);
		fwrite(buf, 1, rlen, fp);
	}while(len < p->size);
	
	printf("\n");
	close(fd);	//�ر�TCP Client
	fclose(fp); //�ر��ļ�
	del_file(p, RECVFILE);	//���ļ��б���ɾ�����չ����ļ�
	return 0;
}

//�����ļ����߳�
void *sendfile_thread(void *arg)
{
	int fd = tcp_fd();	//��ȡTCP_Server�׽ӿ�������
	while(1)
	{
		struct sockaddr_in addr = {AF_INET};
		unsigned int addrlen = sizeof(addr);
		int clifd = accept(fd, (struct sockaddr*)&addr, &addrlen);
		if( clifd<0 )
		{
			perror("accept");
			exit(1);
		}
		while(1)	// ���Ͷ���ļ�
		{
			IPMSG_FILE *p = NULL;
			FILE *fp = NULL;
			IPMSG_USER temp;
			long pkgnum = 0 ;
			char edition[100]=""; 
			long oldpkgnum = 0 ;
			long cmd = 0;
			int filenum = 0;
			char buf[1400]="";
			int sendsize = 0;			
			//����IPMSG_GETFILEDATA
			if(recv(clifd, buf, sizeof(buf), 0)==0)
				break;
			sscanf(buf, "%[^:]:%ld:%[^:]:%[^:]:%ld:%lx:%x",edition, &pkgnum, temp.name, temp.host, &cmd,\
					&oldpkgnum, &filenum);
			//�Ƿ���IPMSG_GETFILEDATA
			if((GET_MODE(cmd)&IPMSG_GETFILEDATA)!=IPMSG_GETFILEDATA)
				break;
			//��ȡ֮ǰ���͵��ļ���Ϣ
			if ((p = getfileinfo(oldpkgnum, filenum))==NULL)
			{
				return NULL;
			}
			if( (fp=fopen(p->name, "r"))==NULL )
			{
				IPMSG_OUT_MSG_COLOR(
				printf("senderror: no such file: %s\n", p->name);
				)
				return NULL;
			}			
			do	//�����ļ�
			{
				int size = fread(buf, 1, sizeof(buf), fp);
				send(clifd, buf, size, 0);
				sendsize += size;
			}while(sendsize < p->size);
			fclose(fp);				//�ر��ļ�
			del_file(p, SENDFILE);	//�ӷ����ļ�������ɾ���ļ�
		}//end wile1	 // ѭ�����Ͷ���ļ�
		close(clifd);	 //�ر��׽ӿڵȴ��¸��û�����
	}//end while
	return NULL;
}

//������Ϣ�̣߳����������ͻ��˷��͵�UDP����
void *recv_msg_thread(void *arg)
{
	while(1)
	{
		char buf[500]="";		
		char edition[100]="";
		struct sockaddr_in addr = {AF_INET};
		unsigned int addrlen = sizeof(addr);
		int len = 0;
		long pkgnum = 0;
		long cmd = 0;
		char msg[100]="";
		int t = 0;
		char *p = NULL;
		IPMSG_USER temp;			
		len = recvfrom(udp_fd(), buf, sizeof(buf), 0, (struct sockaddr*)&addr, &addrlen);
		sscanf(buf, "%[^:]:%ld:%[^:]:%[^:]:%ld",edition, &pkgnum, temp.name, temp.host, &cmd);
		
		p = strrchr(buf, ':');			//���Ҹ�����Ϣ
		memcpy(msg, p+1, len-(p-buf));	//��������Ϣ����msg��
		
		temp.s_addr = addr.sin_addr.s_addr;
		switch(GET_MODE(cmd))
		{
		case IPMSG_BR_ENTRY:
			t = time((time_t *)NULL);
			len = sprintf(buf,"1:%d:%s:%s:%ld:%s",t,user(),host(),IPMSG_ANSENTRY,user());
			sendto(udp_fd(),buf,len,0,(struct sockaddr*)&addr,sizeof(addr));
		case IPMSG_ANSENTRY:
			add_user(temp);
			break;
		case IPMSG_SENDMSG:
			if(msg[0]!=0)
			{
				IPMSG_OUT_MSG_COLOR(
					printf("\r[recv msg from: %s ]#\n%s\n", temp.name, msg);
				)
				write(1,"\rIPMSG:",7);
			}
			if((cmd&IPMSG_SENDCHECKOPT)==IPMSG_SENDCHECKOPT)
			{
				char buf[50]="";
				t = time((time_t *)NULL);
				int len = sprintf(buf,"1:%d:%s:%s:%ld:%ld",t,user(),host(),IPMSG_RECVMSG, pkgnum);
				sendto(udp_fd(),buf,len,0,(struct sockaddr*)&addr,sizeof(addr));
			}
			if((cmd&IPMSG_FILEATTACHOPT)==IPMSG_FILEATTACHOPT)
			{
				char *p = msg+strlen(msg)+1;
				//printf("filemsg=%s\n",p);
				char *fileopt= strtok(p, "\a");		//fileoptָ���һ���ļ�����
				do{	//ѭ����ȡ�ļ���Ϣ
					IPMSG_FILE ftemp;
					sscanf(fileopt, "%d:%[^:]:%lx:%lx", &ftemp.num, ftemp.name, &ftemp.size, &ftemp.ltime);	
					strcpy(ftemp.user, temp.name);
					ftemp.pkgnum = pkgnum;
					add_file(ftemp, RECVFILE);
					fileopt = strtok(NULL, "\a");	//fileoptָ����һ���ļ�����
				}while(fileopt!=NULL);
				IPMSG_OUT_MSG_COLOR(
				printf("\r<<<Recv file from %s!>>>\n", temp.name);
				)
				write(1,"\rIPMSG:",7);
			}
			break;
		case IPMSG_RECVMSG:
			{
				IPMSG_OUT_MSG_COLOR(
				printf("\r%s have receved your msg!\n", temp.name);
				)
				write(1,"\rIPMSG:",7);
			}
			break;			
		case IPMSG_BR_EXIT:
			del_user(temp);
			break;
		default :
			break;			
		}
	}
	return NULL;
}
