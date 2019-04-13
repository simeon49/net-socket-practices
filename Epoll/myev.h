#include <stdio.h>

struct myevent_loop;

/******* copy from sys/epoll.h ********/
/*typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event
{
  uint32_t events;	    // Epoll events 
  epoll_data_t data;    // User data variable
} __EPOLL_PACKED;*/

typedef struct wlist {
    struct wlist *next;
} wlist;

typedef struct myevent {
    wlist next;         // myevent 事件队列使用
    int fd;             // 被监听的fd
    void (*call_back)(struct myevent_loop *ev_loop, struct myevent *ev, int events);    // 事件发生时的回调函数
    int events;         // 监听的事件
    char buff[1024];    // 数据缓存
    int len;            // 数据长度
    long last_active;   // 最后活跃时间
} myevent;

typedef struct myevent_loop {
    int epollfd;        // epoll_create() 返回的fd
    wlist *ev_head;     // myevent 事件队列
} myevent_loop;

#define MAX_EVENTS 10240

void accept_handler(myevent_loop *ev_loop, myevent *ev, int events);

void recv_handler(myevent_loop *ev_loop, myevent *ev, int events);

void send_handler(myevent_loop *ev_loop, myevent *ev, int events);

