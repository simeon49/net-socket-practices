/*
	try_socket
*/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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


int try_socket(int domain, int type, int protocol)
{
	int sockfd;

	if ((sockfd = socket(domain, type, protocol)) == -1) {
		ErrPrint("socket %s\n", strerror(errno));
		return -1;
	}
	return sockfd;
}


int try_bind(int sockfd, const struct sockaddr_in *addr)
{
	if (bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr)) == -1) {
		ErrPrint("bind %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int try_listen(int sockfd, int backlog)
{
	if (listen(sockfd, backlog) == -1) {
		ErrPrint("listen %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int try_connect(int sockfd, struct sockaddr_in *p_address)
{
	fd_set readfds, writefds, exceptfds;
	struct timeval timeout;
	int err, res;
	socklen_t err_len;

	if ((res = connect(sockfd, (struct sockaddr *)p_address, sizeof(struct sockaddr))) == -1) {
		err = errno;
		if (err == EINPROGRESS) {	/*TCP 正在进行握手  等待中*/

			FD_ZERO(&readfds);
			FD_SET(sockfd, &readfds);
			writefds = exceptfds = readfds;
			timeout.tv_sec = 5;
			timeout.tv_usec = 0;

			res = select(sockfd + 1, &readfds, &writefds, &exceptfds, &timeout);
			switch (res) {
			case -1:
				ErrPrint("select %s\n", strerror(errno));
				return -1;
			case 0:		//	time is out
				ErrPrint("connect will be close %s\n", strerror(ETIMEDOUT));
				return -1;
			default:
				/*	http://tianhouse.blog.163.com/blog/static/139695265201132994357640/*/
				/*	使用select监听socket描述符是否可读或者可 写，如果只可写，说明连接成功，可以进行下面的操作。
					如果描述符既可读又可写，分为两种情况，第一种情况是socket连接出现错误（不要问为什么，这是系统
					规定的，可读可写时候有可能是connect连接成功后远程主机断开了连接close(socket)），第二种情
					况是connect连接成 功，socket读缓冲区得到了远程主机发送的数据。需要者通过调用 
					getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len);*/
				if (!FD_ISSET(sockfd, &writefds) || FD_ISSET(sockfd, &exceptfds)) {
					err_len = sizeof(err);
					if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &err_len) == -1) {
						ErrPrint("getsockopt %s\n", strerror(errno));
						return -1;
					}
					ErrPrint("connect will be close %s\n", strerror(err));
					return -1;
				}
			}
		} else {
			ErrPrint("connect %s\n", strerror(err));
			return -1;
		}
	}
	return 0;
}


#define SEND_RETRY_TIME 10000	/*10ms*/
int try_send(int sockfd, const void  *buf, size_t len, unsigned int timeout)
{
	size_t total_sends, nsends;
	int err;
	int time_left;
	char *data;

	data = (char *)buf;
	timeout = timeout > 20? 20:timeout;	//	timeout <= 20s
	time_left = timeout * 1000 *1000;	//	1 s = 10^6 us
	total_sends = 0;
	while (total_sends < len) {
		nsends = send(sockfd, data + total_sends, len - total_sends, MSG_DONTWAIT);
		err = errno;
		switch (nsends){
		case -1:
			if (err == EAGAIN) {
				if (time_left <= 0) {
					ErrPrint("send %s\n", strerror(ETIMEDOUT));
					return -1;
				}
				usleep(SEND_RETRY_TIME);
				time_left -= SEND_RETRY_TIME;
				continue;
			} else {
				ErrPrint("send %s\n", strerror(err));
				return -1;
			}
		default:
			total_sends += nsends;
		}
	}
	return total_sends;
}

int try_recv(int sockfd, void *buf, size_t len, unsigned int timeout)
{
	int nrecvs;
	int err;
	int time_left;
	char *data;

	data = (void *)buf;
	timeout = timeout > 20? 20:timeout;	//	timeout <= 20s
	time_left = timeout * 1000 *1000;	//	1 s = 10^6 us

	if (data == NULL || len <= 0)
		return -1;
	data[0] = '\0';

	while (1) {
		nrecvs = recv(sockfd, data, len, MSG_DONTWAIT);
		err = errno;
		switch (nrecvs){
		case -1:
			if (err == EAGAIN || err == EWOULDBLOCK) {
				if (time_left <= 0) {
					ErrPrint("recv %s\n", strerror(ETIMEDOUT));
					return -1;
				}
				usleep(SEND_RETRY_TIME);
				time_left -= SEND_RETRY_TIME;
				continue;
			} else {
				ErrPrint("recv %s\n", strerror(err));
				return -1;
			}
		default:
			return nrecvs;
		}
	}
}
