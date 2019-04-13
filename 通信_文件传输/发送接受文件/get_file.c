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

void init_signals(void)
{

}

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	int fd_tcp;
	int opt_value = 1;
	char buf[1024] = "./test.pdf";

	if ((fd_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		ErrPrintQ("socket");

	if ((setsockopt(fd_tcp, SOL_SOCKET, SO_REUSEADDR, (void *)&opt_value, sizeof(opt_value))) == -1)
		ErrPrintQ("setsockopt");

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(TCP_PORT);
	if (inet_aton("127.0.0.2", &addr.sin_addr) == 0)
		ErrPrintQ("inet_aton");

	if (connect(fd_tcp, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		ErrPrintQ("connect");

	init_signals();

	if (send(fd_tcp, buf, strlen(buf), 0) == -1)
		ErrPrintQ("send");

	FILE *fp;
	if ((fp = fopen("temp.pdf", "w")) == NULL)
		ErrPrintQ("fopen");

	long size;
	long size_total = 0l;
	do {
		if ((size = recv(fd_tcp, buf, sizeof(buf), 0)) == -1) {
			perror("recv");
			break;
		}
		//sleep(1);
		size_total += size;
		fwrite(buf, 1, size, fp);
	} while (size > 0);

	if (fclose(fp) != 0)
		ErrPrintQ("fclose");
	close(fd_tcp);

	return 0;
}
