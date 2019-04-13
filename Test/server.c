#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int block_socket_server(int port)
{
    int res;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    // 端口重用
    int value = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&value, sizeof(value));
    res = bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (res < 0) {
        printf("Err bind(): %s\n", strerror(errno));
        exit(-1);
    }
    sleep(1000);
    res = listen(listen_fd, 2);
    while(1) {
        char buff[100];
        int addr_len =sizeof(addr);
        int fd = accept(listen_fd, (struct sockaddr *)&addr, &addr_len);
        if (fd < 0) {
            printf("accept err: %d\n", fd);
            break;
        } else {
            printf("connection from %s:%d\n",
                inet_ntop(AF_INET, &addr.sin_addr, buff, sizeof(buff)),
                ntohs(addr.sin_port));
            int recvlowat = 32; // 接收的低潮值,只有缓存区的数据量大于等于改值时才可读(默认为1)
            setsockopt(fd, SOL_SOCKET, SO_RCVLOWAT, (const char *)&recvlowat, sizeof(recvlowat));
            while (1) {
                char recv_buf[1024];
                bzero(recv_buf, sizeof(recv_buf));
                int ret = read(fd, recv_buf, sizeof(recv_buf));
                if (ret < 0) {
                    printf("read err: %s\n", strerror(errno));
                    close(fd);
                    break;
                } else if (ret == 0) {
                    printf("client close.\n");
                    close(fd);
                    break;
                } else {
                    printf("recv: %s\n", recv_buf);
                    write(fd, "OK", 3);
                }
            }
        }
    }
    return 0;
}



int main()
{
    int port = 8080;
    block_socket_server(port); 
    return 0;
}

