#define _FILE_OFFSET_BITS 64	//	To suporrt file size >4G
#define _LARGE_FILE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <assert.h>		//	does it still work in Android?

#include "try_socket.h"
#include "common.h"


/**/
void *a_service_thread(void *arg)
{
	int client_sockfd, file_fd;
	char file_name[1024] = "";
	size_t curr_size = 0, file_size = 0;
	char recv_buf[RECV_BUFSIZE], send_buf[SEND_BUFSIZE];
	int nreads, nsends;

	client_sockfd = (int)arg; 

	nreads = try_recv(client_sockfd, recv_buf, sizeof(recv_buf), TRY_RECV_TIMEOUT);
	if (nreads == -1) {

		goto a_service_err;

	} else if (nreads == 0) {	//当没有数据可读时表示 客户端关闭了端口

		Print("This tcp connect(fd:%d) was closed by client\n", client_sockfd);
		goto a_service_err;

	} else {

		sscanf(recv_buf, "NAME:%s SIZE:%d", file_name, &file_size);
		if (strcmp(file_name, "") == 0 || file_size == 0) {
			Print("Cannot analysis recv_buf: %s\n", recv_buf);
			goto a_service_err;
		}
		Print("FileName: %s, FileSize: %d\n", file_name, file_size);
		file_fd = open(file_name, O_WRONLY);
		if (file_fd != -1) {
			curr_size = lseek(file_fd, 0, SEEK_END);
			if (curr_size == -1) {
				ErrPrint("lseek %s\n", strerror(errno));
				close(file_fd);
				goto a_service_err;
			} else if (curr_size > file_size) {
				Print("File %s now [%d] bigger than [%d]\n", file_name, curr_size, file_size);
				close(file_fd);
				goto a_service_err;
			} else {
				memset(send_buf,'\0', sizeof(send_buf));
				nsends = sprintf(send_buf, "FROM:%d", curr_size);
				if (try_send(client_sockfd, send_buf, nsends, TRY_SEND_TIMEOUT) != nsends) {
					goto a_service_err;
				}
				Print("StarFrom: %d\n", curr_size);
				file_size -= curr_size;
			}
		} else {
			file_fd = open(file_name, O_WRONLY|O_CREAT, 0644);
			if (file_fd == -1) {
				ErrPrint("open %s\n", strerror(errno));
				goto a_service_err;
			}
			memset(send_buf,'\0', sizeof(send_buf));
			nsends = sprintf(send_buf, "FROM:%d", curr_size);
			if (try_send(client_sockfd, send_buf, nsends, TRY_SEND_TIMEOUT) != nsends) {
				goto a_service_err;
			}
		}
	}

	while (file_size > 0) {

		nreads = try_recv(client_sockfd, recv_buf, sizeof(recv_buf), TRY_RECV_TIMEOUT);
		if (nreads == -1) {

			Print("Net error check the net\n");
			close(file_fd);
			goto a_service_err;

		} else if (nreads == 0) {	//当没有数据可读时表示 客户端关闭了端口

			Print("This tcp connect(fd:%d) was closed by client\n", client_sockfd);
			close(file_fd);
			goto a_service_err;

		} else {

			if (write(file_fd, recv_buf, nreads) != nreads) {
				ErrPrint("write %s\n", strerror(errno));
				close(file_fd);
				goto a_service_err;
			}
			file_size -= nreads;
		}

	}

	close(file_fd);
	Print("%s complete\n", file_name);
a_service_err:
	close(client_sockfd);
	return ((void *)0);
}



int main(int argc, char *argv[])
{
	int server_sockfd = -1, client_sockfd = -1, fd = -1;
	struct sockaddr_in addr;
	socklen_t addr_len;

	pthread_t service_pthread;
	struct timeval daly_time;
	fd_set readfds, r_tmpfds;
	int res, opt;

	if ((server_sockfd = try_socket(AF_INET, SOCK_STREAM, 0))  == -1)
		pthread_exit((void *)-1);

	/*	http://baike.baidu.com/view/569217.htm	*/
	/*	这个套接字选项通知内核，如果端口忙，但TCP状态位于 TIME_WAIT ，可以重用 
		端口。如果端口忙，而TCP状态位于其他状态，重用端口时依旧得到一个错误信息， 
		指明"地址已经使用中"。如果你的服务程序停止后想立即重启，而新套接字依旧 
		使用同一端口，此时 SO_REUSEADDR 选项非常有用。必须意识到，此时任何非期 
		望数据到达，都可能导致服务程序反应混乱，不过这只是一种可能，事实上很不 
		可能.    "bind: address in use"*/
	opt = 1;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		ErrPrint("setsockopt %s\n", strerror(errno));
		goto server_err;
	}

	/*	为 TCP socket 禁用 Nagle 算法   Nagle算法通过将未确认的数据存入缓冲
		区直到蓄足一个包一起发送的方法，来减少主机发送的零碎小数据包的数目。但对
		于某些应用来说，这种算法将降低系统性能。*/
	opt = 1;
	if (setsockopt(server_sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1) {
		ErrPrint("setsockopt %s\n", strerror(errno));
		goto server_err;
	}

	opt = RECV_BUFSIZE;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) == -1) {
		ErrPrint("setsockopt %s\n", strerror(errno));
		goto server_err;
	}

	opt = SEND_BUFSIZE;
	if (setsockopt(server_sockfd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) == -1) {
		ErrPrint("setsockopt %s\n", strerror(errno));
		goto server_err;
	}

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(8900);	//	Server Port is 9000
	addr_len = sizeof(addr);

	if (try_bind(server_sockfd, &addr) == -1)
		goto server_err;

	if (try_listen(server_sockfd, 5)== -1)
		goto server_err;

	signal(SIGPIPE,SIG_IGN);
	
	FD_ZERO(&readfds);
	FD_SET(server_sockfd, &readfds);
	while (1) {

		daly_time.tv_sec = 1;
		daly_time.tv_usec = 0;

		/*	select函数有可能会改变被监听的文件描述符集 所以应该每次都要备份	*/
		r_tmpfds = readfds;

		res = select(FD_SETSIZE, &r_tmpfds, (fd_set *)NULL, (fd_set *)NULL, &daly_time);
		if (res == -1) {

			ErrPrint("select %s\n", strerror(errno));
			goto server_err;

		} else if (res == 0) {	/*时间到 */

		} else {

			/*	检测每一个描述符	*/
			for (fd = 0; fd < FD_SETSIZE; fd ++) {
				if (FD_ISSET(fd, &r_tmpfds)) {
					if (fd == server_sockfd) {	//当server_sockfd可读时 表示有新的客户链接请求

						if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&addr, &addr_len)) == -1){
							ErrPrint("accept %s\n", strerror(errno));
							goto server_err;
						}
						FD_SET(client_sockfd, &readfds);
						Print("New client with port %d  fd:%d\n", ntohs(addr.sin_port), client_sockfd);

					} else {
						
						Print("Create a service thread for fd:%d\n", client_sockfd);

						res = pthread_create(&service_pthread, NULL, a_service_thread, (void *)fd);
						if (res != 0) {
							ErrPrint("pthread_create(a_service_thread) %s\n", strerror(res));
							close(fd);
						}

						/*	抛弃线程	*/
						res = pthread_detach(service_pthread);
						if (res != 0) {
							ErrPrint("pthread_detach(a_service_thread) %s\n", strerror(res));
							close(fd);
						}

						FD_CLR(fd, &readfds);

					}
				}
			}

		}
	}
server_err:
	shutdown(server_sockfd, SHUT_RDWR);
	close(server_sockfd);
	return 0;
}
