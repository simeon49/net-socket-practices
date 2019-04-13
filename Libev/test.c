#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ev.h>

void setnoblock(int fd) 
{
    int flags = fcntl(fd, F_GETFL, 0); 
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void accept_action(struct ev_loop *main_loop, ev_io *w, int e);
void read_action(struct ev_loop *main_loop, ev_io *w, int e);
void write_action(struct ev_loop *main_loop, ev_io *w, int e);

void accept_action(struct ev_loop *main_loop, ev_io *w, int e)
{
    printf("[accept action]: new connect:\n");
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(w->fd, (struct sockaddr *)&client_addr, &addr_len);
    setnoblock(client_fd);
    ev_io *read_w = (ev_io *)malloc(sizeof(ev_io));
    ev_io_init(read_w, read_action, client_fd, EV_READ);
    ev_io_start(main_loop, read_w);
    printf("\t->client fd: %d\n", client_fd);
}

void read_action(struct ev_loop *main_loop, ev_io *w, int e)
{
    printf("[read action]: client request:\n");
    char buff[1024];
    bzero(buff, sizeof(buff));
    ssize_t len = read(w->fd, buff, sizeof(buff));
    printf("\t->request: %s\n", buff);
    ev_io *write_w = (ev_io *)malloc(sizeof(ev_io));
    ev_io_init(write_w, write_action, w->fd, EV_WRITE);
    ev_io_start(main_loop, write_w);
}

void write_action(struct ev_loop *main_loop, ev_io *w, int e)
{
    printf("[write action]: response.\n");
    char buff[1024] = "OK";
    ssize_t len = write(w->fd, buff, strlen(buff));
    ev_io_stop(main_loop, w);
}

void timeout_action(struct ev_loop *main_loop, ev_timer *w, int e)
{
    puts("time out.");
    //ev_break(EV_A_ EVBREAK_ONE);
}

void signal_action(struct ev_loop *main_loop, ev_signal *w, int e)
{
    printf("[signal_action]: recv signal: %d\n", w->signum);
    ev_signal_stop(main_loop, w);
    ev_break(main_loop, EVBREAK_ONE);
}

int main()
{
    int res;
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket fail:");
        exit(-1);
    }

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(7979);
	addr.sin_addr.s_addr = INADDR_ANY;
	res = bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (res == -1) {
        perror("bind fail:");
        exit(-1);
    }
    res = listen(listen_fd, 10);
    if (res == -1) {
        perror("listen fail:");
        exit(-1);
    }
    setnoblock(listen_fd);
    struct ev_loop *main_loop = ev_loop_new(EVBACKEND_EPOLL);
    ev_io listen_w;
    ev_io_init(&listen_w, accept_action, listen_fd, EV_READ);
    ev_io_start(main_loop, &listen_w);

    /*ev_signal signal_w;
    ev_init(&signal_w, signal_action);
    ev_signal_set(&signal_w, SIGINT);

    ev_timer timeout_w;
    ev_timer_init(&timeout_w, timeout_action, 4.9, 0);
    ev_timer_start(main_loop, &timeout_w);*/

    ev_run(main_loop, 0);
    return 0;
}
