/* ************************************************************************
 *       Filename:  ipmsg.c
 *    Description:  main.c
 *        Version:  1.0
 *        Created:  2010��03��08�� 10ʱ28��50��
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 
 
 *     Maintainer: wenhao  v1.1 2011-11-10
				   1. ����������״̬ʱ,����ո���δ����BUG
				   2. �����յ�SENDCHECKOPTʱ�ظ������Ĭ��Ϊ0��BUG
				   3. ����help��ӡ���ݹ淶����
 * ************************************************************************/
#include "myinclude.h"
#include "time.h"
#include "communication.h"
#include "user_manager.h"
#include "user_interface.h"

int main(int argc, char *argv[])
{
	pthread_t tid;	
	online("Sunplusapp", "root_teacher");
	
	//������Ϣ�̣߳����������ͻ��˷��͵�UDP����
	pthread_create(&tid, NULL, recv_msg_thread, NULL);
	
	//�û������̣߳������û����������
	pthread_create(&tid, NULL, user_interface, NULL);
	
	//�����ļ��̣߳��ȴ������ͻ��˽����ļ����������䴫���ļ�
	pthread_create(&tid, NULL, sendfile_thread, NULL);
	//���̲߳����˳�
	
	pthread_join(tid, NULL);
	return 0;	
}
