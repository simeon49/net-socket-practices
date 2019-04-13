/* ************************************************************************
 *       Filename:  soket_client.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2013年02月14日 
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>

#include "socket_headfile.h"

void inet_sockt(int num){
	int		i;
	char	ch = 250;
	int	sockfd;
	struct sockaddr_in address;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		errexit("socket");
	}
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(SEVERPORT);
	/*
	assert(inet_pton(AF_INET, "192.168.0.100", (void *)&address.sin_addr.s_addr) == 1);
	printf("%x\n", address.sin_addr.s_addr);*/
	/*
	address.sin_addr.s_addr = inet_addr(INADDR_ANY);
	 */
	address.sin_addr.s_addr = inet_addr(SEVERIP);
	printf("%x\n", address.sin_addr.s_addr);
	if(connect(sockfd, (struct sockaddr *)&address, sizeof(address)) == -1){
		errexit("connect");
	}
	for(i = 0; i < num; i ++){
		if(write(sockfd, &ch, 1) == -1){
			errexit("write");
		}
		if(read(sockfd, &ch, 1) < 0){
			errexit("read");
		}
		printf(" Process ID %d receive %u\n", getpid(), (unsigned char)ch);
		ch ++;
		sleep(1);
	}
	close(sockfd);
}

int main(int argc, char *argv[])
{
	if(argc != 2){
		printf("需要1参数 告知向服务器发送数据的次数\n");
		exit(0);
	}
	printf("此次将发送%c次数据\n", *(char *)argv[1]);
	inet_sockt(*(char *)argv[1] - '0');
	return 0;
}

