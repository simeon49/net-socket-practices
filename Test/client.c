#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

void set_noblock(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFD, flags|O_NONBLOCK);
}

void set_block(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    fcntl(fd, F_SETFD, flags&~O_NONBLOCK);
}

#define ASSERT(res) do{if (res < 0){printf("err line:%d, errno:%d\n", __LINE__, res); exit(-1);}}while(0)

int main()
{
    int res;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    bzero(&addr, sizeof(struct sockaddr_in));
    ASSERT(bind(fd, (struct sockaddr*)&addr, sizeof(addr)));
    addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &addr.sin_addr);
    addr.sin_port = htons(9000);
    time_t time_start = time(NULL);    
    ASSERT(connect(fd, (struct sockaddr *)&addr, sizeof(addr)));
    printf("time use: %ld\n", time(NULL) - time_start);
    char send_buf[1024] = "1234567\n";
    char recv_buf[1024];
    int i;
    for (i = 0; i < 7; i ++) {
        ASSERT(send(fd, send_buf, strlen(send_buf)+1, 0));
        sleep(1);
    }
    shutdown(fd, SHUT_RD);
    sleep(1);
    ASSERT(recv(fd, recv_buf, sizeof(recv_buf), 0));
    printf("recv_buf: %s", recv_buf);
    //close(fd);
    return 0;
}

