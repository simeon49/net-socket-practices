/* ************************************************************************
 *       Filename:  communicatioh.h
 *    Description:  
 *        Version:  1.0
 *        Created:  2010��03��08�� 10ʱ28��50��
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/
#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_

#include "myinclude.h"
#define PORT 2425

//������Ϣ�̣߳����������ͻ��˷��͵�UDP����
void *recv_msg_thread(void *arg);
//�����ļ��̣߳��ȴ������ͻ��˽����ļ����������䴫���ļ�
void *sendfile_thread(void *arg);

//����
void online(char *user, char *host);
//����
void ipmsg_exit(void);

//������Ϣ
void msg_send(char *msg, int len, struct sockaddr_in addr);
//�����ļ�(����Ϊ�����ļ��б��е����)
int recvfile(int id);

//��ȡ�û���
char *user(void);
//��ȡ������
char *host(void);
//��ȡUDP������
int udp_fd(void);

#endif	//_COMMUNICATION_H_

