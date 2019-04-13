#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>  
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include "myev.h"

void wlist_add(wlist **head, wlist *elem)
{
    elem->next = *head;
    *head = elem;
}

void wlist_del(wlist **head, wlist *elem)
{
    while (*head != NULL) {
        if (*head == elem) {
            *head = elem->next;
            break;
        }
        *head = (*head)->next;
    }
}

myevent_loop *EventNew()
{
    myevent_loop *ev_loop = (myevent_loop *)malloc(sizeof(myevent_loop));
    bzero(ev_loop, sizeof(myevent_loop));
    ev_loop->epollfd = epoll_create(1);
    if (ev_loop->epollfd < 0) {
        printf("EventNew failed: %s\n", strerror(errno));
        return NULL;
    }
    ev_loop->ev_head = NULL;
    return ev_loop;
}

void EventSet(myevent *ev, int fd, void(*call_back)(struct myevent_loop *ev_loop, myevent *ev, int events))
{
    bzero(ev, sizeof(myevent));
    ev->fd = fd;
    ev->call_back = call_back;
    ev->last_active = time(NULL);
}

int EventAdd(myevent_loop *ev_loop, int events, myevent *ev)
{
    struct epoll_event epv;
    bzero(&epv, sizeof(struct epoll_event));
    epv.events = events;
    epv.data.ptr = ev;
    ev->events = events;
    int op = EPOLL_CTL_ADD;
    if (epoll_ctl(ev_loop->epollfd, op, ev->fd, &epv) <0 ) {
        if (errno == EEXIST) {
            // do sth here
        } else {
            printf("EventAdd failed[fd=%d]: %s\n", ev->fd, strerror(errno));
            return -1;
        }
    }
    wlist_add(&ev_loop->ev_head, (wlist *)ev);
    printf("EventAdd OK[fd=%d]\n", ev->fd);
    return 0;
}

int EventDel(myevent_loop *ev_loop, myevent *ev)
{
    struct epoll_event epv = {0,{0}};
    epv.data.ptr=ev;
    epv.data.fd = ev->fd;
    int op = EPOLL_CTL_DEL;
    epoll_ctl(ev_loop->epollfd, op, ev->fd, &epv);
    wlist_del(&ev_loop->ev_head, (wlist *)ev);
    return 0;
}

void EventRun(myevent_loop *ev_loop)
{
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int i;
        long now = time(NULL);
        myevent *ev = (myevent *)ev_loop->ev_head;
        while (ev != NULL) {
            // printf("-->fd=%d\n", ev->fd);
            if (now - ev->last_active > 60) {
                // timeout
            }
            ev = (myevent *)ev->next.next;
        }
        int ev_nums = epoll_wait(ev_loop->epollfd, events, MAX_EVENTS, 1e3);
        if (ev_nums < 0) {
            printf("epoll_wait failed: %s\n", strerror(errno));
            exit(-1);
        }
        for (i = 0; i < ev_nums; i ++) {
            myevent *ev = (myevent *)events[i].data.ptr;
            //printf("fd=%d ev=0x%x\n", events[i].data.fd, ev);
            ev->last_active = time(NULL);
            ev->call_back(ev_loop, ev, events[i].events);
        }
    }
}

void accept_handler(myevent_loop *ev_loop, myevent *ev, int events)
{
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int client_fd = accept(ev->fd, (struct sockaddr *)&client_addr, &len);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("accept failed: %s\n", strerror(errno));
        }
        return ;
    }
    printf("New client[%s:%d] connect.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    myevent *recv_ev = (myevent *)malloc(sizeof(myevent));
    EventSet(recv_ev, client_fd, recv_handler);
    EventAdd(ev_loop, EPOLLIN|EPOLLET, recv_ev);
}

void recv_handler(myevent_loop *ev_loop, myevent *ev, int events)
{
    bzero(ev->buff, sizeof(ev->buff));
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    getpeername(ev->fd, (struct sockaddr *)&client_addr, &len);
    int ret = recv(ev->fd, ev->buff, sizeof(ev->buff), 0);
    if (ret < 0) {
        printf("Recv from [%s:%d] failed: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), strerror(errno));
        return ;
    } else if (ret == 0) {
        printf("Client [%s:%d] closed.\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        EventDel(ev_loop, ev);
        close(ev->fd);
        free(ev);
    } else {
        ev->len = ret;
        printf("Recv from [%s:%d] data(%d): %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), ev->len, ev->buff);
        EventDel(ev_loop, ev);
        free(ev);
        myevent *send_ev = (myevent *)malloc(sizeof(myevent));
        EventSet(send_ev, ev->fd, send_handler);
        EventAdd(ev_loop, EPOLLOUT|EPOLLET, send_ev);
    }
}

void send_handler(myevent_loop *ev_loop, myevent *ev, int events)
{
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    getpeername(ev->fd, (struct sockaddr *)&client_addr, &len);
    char send_buf[1024] = "OK";
    int ret = send(ev->fd, send_buf, strlen(send_buf), 0);
    if (ret < 0) {
        printf("Send to [%s:%d] failed: %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), strerror(errno));
        return ;
    } else {
        printf("Send to [%s:%d] data(%d): %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), strlen(send_buf), send_buf);
    }
    EventDel(ev_loop, ev);
    free(ev);
    myevent *recv_ev = (myevent *)malloc(sizeof(myevent));
    EventSet(recv_ev, ev->fd, recv_handler);
    EventAdd(ev_loop, EPOLLIN|EPOLLET, recv_ev);
}

void set_nonblock(int fd)
{
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

int main()
{
    myevent_loop *ev_loop = EventNew();
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblock(listen_fd);
    struct sockaddr_in addr;
    bzero(&addr, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8000);
    bind(listen_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
    listen(listen_fd, 100);
    myevent *ev = (myevent *)malloc(sizeof(myevent));
    EventSet(ev, listen_fd, accept_handler);
    EventAdd(ev_loop, EPOLLIN|EPOLLET, ev);
    EventRun(ev_loop);
    return 0;
}

