/* ************************************************************************
 *       Filename:  file_transfer.c
 *    Description:
 *        Version:  1.0
 *        Created:  2013年06月05日 13时19分17秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (),
 *        Company:
 * ************************************************************************/


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#define ErrPrintQ(_msg) do{fprintf(stderr, "[Err]%s: %s\n", _msg, strerror(errno)); exit(EXIT_FAILURE);}while(0)
#define TCP_PORT 9000
#define MAX_CONNECT 10
#define UPDATA_NFDS(fd, nfds) ((nfds) = ((fd) > (nfds)? (fd):(nfds)))

fd_set readfds;
//fd_set writefds;
int nfds = 0;

void init_signals(void)
{

}

void *transfer_file_thread(void *arg)
{
	char buf[10];
	FILE *fp;
	int  fd_client;
	long file_size;
	long total_size = 0L;
	long size;

	fd_client = (int)arg;
	memset(buf, '\0', sizeof(buf));
	if (recv(fd_client, buf, sizeof(buf), 0) == -1) {
		fprintf(stderr, "[Err]fd:%d recv", fd_client);
		//FD_CLR(fd_client, &readfds);
		pthread_exit(NULL);
	}
	
	printf("fd:%d Ask Dowload file: %s\n", fd_client, buf);
	if ((fp = fopen(buf, "r")) == NULL) {
		fprintf(stderr, "[Err]fopen %s fail: %s\n", buf, strerror(errno));
		//FD_CLR(fd_client, &readfds);
		pthread_exit(NULL);
	}
	if (fseek(fp, 0L, SEEK_END) == -1) {
		fprintf(stderr, "[Err]fseek %s fail: %s\n", buf, strerror(errno));
		//FD_CLR(fd_client, &readfds);
		pthread_exit(NULL);
	}
	if ((file_size = ftell(fp)) == -1) {
		fprintf(stderr, "[Err]ftell %s fail: %s\n", buf, strerror(errno));
		//FD_CLR(fd_client, &readfds);
		pthread_exit(NULL);
	}
	if (fseek(fp, 0L, SEEK_SET) == -1) {
		fprintf(stderr, "[Err]fseek %s fail: %s\n", buf, strerror(errno));
		//FD_CLR(fd_client, &readfds);
		pthread_exit(NULL);
	}
	printf("fd:%d Transmiting file: %s ...\n", fd_client, buf);
	do {
		memset(buf, '\0', sizeof(buf));
		if ((size = fread(buf, 1, sizeof(buf), fp)) < sizeof(buf)) {
			if (feof(fp)) {
				printf("fd:%d file to end\n", fd_client);
			} else {
				fprintf(stderr, "[Err]fread %s\n", strerror(errno));
				//FD_CLR(fd_client, &readfds);
				pthread_exit(NULL);
			}
		}
		//printf("->%s\n", buf);
		if (send(fd_client, buf, size, 0) == -1) {
			fprintf(stderr, "[Err]send  %s\n", strerror(errno));
			//FD_CLR(fd_client, &readfds);
			pthread_exit(NULL);
		}
		total_size += size;
	} while(total_size < file_size);
	printf("fd:%d Transmit complet!\n", fd_client);
	//FD_CLR(fd_client, &readfds);
	fclose(fp);
	close(fd_client);
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	int fd_transfer_tcp;
	int opt_value = 1;
	fd_set testfds;

	if ((fd_transfer_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		ErrPrintQ("socket");

	if ((setsockopt(fd_transfer_tcp, SOL_SOCKET, SO_REUSEADDR, (void *)&opt_value, sizeof(opt_value))) == -1)
		ErrPrintQ("setsockopt");

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(TCP_PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd_transfer_tcp, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		ErrPrintQ("bind");

	if (listen(fd_transfer_tcp, MAX_CONNECT) == -1)
		ErrPrintQ("listen");

	init_signals();

	FD_ZERO(&readfds);
	//FD_ZERO(&writefds);
	FD_SET(fd_transfer_tcp, &readfds);
	UPDATA_NFDS(fd_transfer_tcp, nfds);

	while (1) {
		int fd;
		int fd_client;
		struct sockaddr_in addr_client;

		pthread_t pthread_transfer;
		pthread_attr_t pthread_attr;

		socklen_t addlen;
		addlen = sizeof(addr_client);
		testfds = readfds;
		if (select(nfds+1, &testfds, (fd_set *)NULL, (fd_set *)NULL, (struct timeval*)NULL) == -1)
			ErrPrintQ("select");
		for (fd = 0; fd < nfds + 1; fd ++) {
			if (FD_ISSET(fd, &testfds)){
				if (fd == fd_transfer_tcp) {
					if ((fd_client = accept(fd_transfer_tcp, (struct sockaddr *)&addr_client, &addlen)) == -1) {
						perror("accept");
						continue;
					}
					fprintf(stdout, "New connect with IP:%s Port:%d fd:%d\n",
							inet_ntoa(addr_client.sin_addr), ntohs(addr_client.sin_port), fd_client);
					FD_SET(fd_client, &readfds);
					UPDATA_NFDS(fd_client, nfds);
				} else {
					printf("Request from fd:%d\n", fd);
					/*
					 *	创建一个脱离状态线程处理 文件传输
					 */
					if (pthread_attr_init(&pthread_attr) != 0)
						ErrPrintQ("pthread_attr_init");
					if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0)
						ErrPrintQ("pthread_attr_setdetachstate");
					if (pthread_create(&pthread_transfer, &pthread_attr, transfer_file_thread, (void *)fd) != 0)
						ErrPrintQ("pthread_create");
					pthread_attr_destroy(&pthread_attr);

					FD_CLR(fd, &readfds);
				}
			}
		}
	}
	return 0;
}
