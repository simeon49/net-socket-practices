#define _FILE_OFFSET_BITS 64	//	To suporrt file size >4G
#define _LARGE_FILE
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <assert.h>		//	does it still work in Android?

#include "try_socket.h"
#include "common.h"

void useage(void){
	fprintf(stderr, "[ip] [port] [file_path]\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	char *file_path, *file_name, *server_ip;
	size_t bytes_need_to_send, file_total_size, curr_size = 0;
	int file_fd, server_port;
	struct stat file_stat;

	int	sockfd;
	struct sockaddr_in address;
	int flags, k, opt, nreads, temp_precent, total_precent = 0;
	char send_buf[SEND_BUFSIZE], recv_buf[RECV_BUFSIZE];

	if (argc != 4) {
		useage();
	}
	server_ip = argv[1];
	server_port = atoi(argv[2]);
	file_path = argv[3];
	file_name = strrchr(file_path, '/');
	if (file_name == NULL)
		file_name = file_path;
	else 
		file_name ++;

	file_fd = open(file_path, O_RDONLY);
	if (file_fd == -1) {
		ErrPrint("open %s\n", strerror(errno));
		return -1;
	}

	if (fstat(file_fd, &file_stat) == -1) {
		ErrPrint("fstat %s\n", strerror(errno));
		close(file_fd);
		return -1;
	}
	bytes_need_to_send = file_total_size = file_stat.st_size;

	Print("Name: %s, Size:%d\n", file_name, file_total_size);

	while (bytes_need_to_send > 0) {
		
		sleep(1);

		bytes_need_to_send = file_total_size;
		if ((sockfd = try_socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			continue;
		}

		/*	为 TCP socket 禁用 Nagle 算法   Nagle算法通过将未确认的数据存入缓冲
			区直到蓄足一个包一起发送的方法，来减少主机发送的零碎小数据包的数目。但对
			于某些应用来说，这种算法将降低系统性能。*/
		opt = 1;
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&opt, sizeof(opt)) == -1) {
			goto execute_end;
		}
		
		/*	设置缓存区大小	*/
		opt = RECV_BUFSIZE;
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) == -1) {
			goto execute_end;
		}

		opt = SEND_BUFSIZE;
		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) == -1) {
			goto execute_end;
		}

		/*设置为非阻塞模式*/
		if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1) {
			goto execute_end;
		}
		if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
			goto execute_end;
		}

		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(server_port);
		address.sin_addr.s_addr = inet_addr(server_ip);

		if (try_connect(sockfd, &address) == -1) {
			goto execute_end;
		}
		memset(send_buf, '\0', sizeof(send_buf));
		snprintf(send_buf, sizeof(send_buf), "NAME:%s SIZE:%d", file_name, file_total_size);
		if (try_send(sockfd, send_buf, strlen(send_buf), TRY_SEND_TIMEOUT) == -1) {
			goto execute_end;
		}

		nreads = try_recv(sockfd, recv_buf, sizeof(recv_buf), TRY_RECV_TIMEOUT);
		if (nreads == -1) {

			goto execute_end;

		} else if (nreads == 0) {	//当没有数据可读时表示 客户端关闭了端口

			Print("This tcp connect was closed by service\n");
			goto execute_end;

		} else {

			sscanf(recv_buf, "FROM:%d", &curr_size);
			lseek(file_fd, curr_size, SEEK_SET);
			Print("StartFrom: %d\n", curr_size);
			bytes_need_to_send -= curr_size;
		}

		Print("file_total_size:%d, bytes_need_to_send:%d\n",file_total_size, bytes_need_to_send);
		sleep(2);

		total_precent = 0;
		while(bytes_need_to_send > 0) {
			nreads = read(file_fd, send_buf, sizeof(send_buf));
			if (nreads == -1) {
				ErrPrint("read %s\n", strerror(errno));
				goto execute_end;
			}
			if (try_send(sockfd, send_buf, nreads, TRY_SEND_TIMEOUT) != nreads) {
				goto execute_end;
			}
			bytes_need_to_send -= nreads;
			temp_precent = total_precent;
			total_precent = (file_total_size - bytes_need_to_send) / (file_total_size / 1000);
			//Print("temp_precent:%d total_precent:%d\n", temp_precent, total_precent);
			for (k = temp_precent; k < total_precent; k ++) {
				putc('#', stderr);
			}
		}

	execute_end:
		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);
	}
	close(file_fd);
	return 0;
}
