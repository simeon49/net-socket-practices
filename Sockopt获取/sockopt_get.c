#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

union val {
    int     i_val;
    long    l_val;
    char    c_val[10];
    struct linger   linger_val;
    struct timeval  timeval_val;
} val;

static char * sock_str_flag(union val *, int);
static char * sock_str_int(union val *, int);
static char * sock_str_linger(union val *, int);
static char * sock_str_timeval(union val *, int);

struct sock_opts {
    char    *opt_str;
    int     opt_level;
    int     opt_name;
    char    *(*opt_val_str)(union val *, int);
} sock_opts[] = {
    "SO_BROADCAST", SOL_SOCKET, SO_BROADCAST,   sock_str_flag,
    "SO_DEBUG",     SOL_SOCKET, SO_DEBUG,       sock_str_flag,
    "SO_DONTROUTE", SOL_SOCKET, SO_DONTROUTE,   sock_str_flag,
    "SO_ERROR",     SOL_SOCKET, SO_ERROR,       sock_str_int,
    "SO_KEEPALIVE", SOL_SOCKET, SO_KEEPALIVE,   sock_str_flag,
    "SO_LINGER",    SOL_SOCKET, SO_LINGER,      sock_str_linger,
    "SO_OOBINLINE", SOL_SOCKET, SO_OOBINLINE,   sock_str_flag,
    "SO_RCVBUF",    SOL_SOCKET, SO_RCVBUF,      sock_str_int,
    "SO_SNDBUF",    SOL_SOCKET, SO_SNDBUF,      sock_str_int,
    "SO_RCVLOWAT",  SOL_SOCKET, SO_RCVLOWAT,    sock_str_int,
    "SO_SNDLOWAT",  SOL_SOCKET, SO_SNDLOWAT,    sock_str_int,
    "SO_RCVTIMEO",  SOL_SOCKET, SO_RCVTIMEO,    sock_str_timeval,
    "SO_SNDTIMEO",  SOL_SOCKET, SO_SNDTIMEO,    sock_str_timeval,
    "SO_REUSEADDR", SOL_SOCKET, SO_REUSEADDR,   sock_str_flag,
    "SO_REUSEPORT", SOL_SOCKET, SO_REUSEPORT,   sock_str_flag,
    "SO_TYPE",      SOL_SOCKET, SO_TYPE,        sock_str_int,
    "IP_TOS",       IPPROTO_IP, IP_TOS,         sock_str_int,
    "IP_TTL",       IPPROTO_IP, IP_TTL,         sock_str_int,
    "TCP_MAXSEG",   IPPROTO_TCP,TCP_MAXSEG,     sock_str_int,
    "TCP_NODELAY",  IPPROTO_TCP,TCP_NODELAY,    sock_str_flag,
    NULL,           0,          0,              NULL,
};

static char strres[128];
char *sock_str_flag(union val *p, int len)
{
    snprintf(strres, sizeof(strres), "%s", (p->i_val == 0?"off":"on"));
    return strres;
}

char *sock_str_int(union val *p, int len)
{
    snprintf(strres, sizeof(strres), "%d", p->i_val);
    return strres;
}

char *sock_str_linger(union val *p, int len)
{
    snprintf(strres, sizeof(strres), "l_onoff=%d, l_linger=%d", p->linger_val.l_onoff, p->linger_val.l_linger);
    return strres;
}

char *sock_str_timeval(union val *p, int len)
{
    snprintf(strres, sizeof(strres), "tv_sec=%ld, tv_usec=%ld", p->timeval_val.tv_sec, p->timeval_val.tv_usec);
    return strres;
}


int main()
{
    int fd, len;
    struct sock_opts *ptr;
    fd = socket(AF_INET, SOCK_STREAM, 0);
    for (ptr = sock_opts; ptr->opt_str != NULL; ptr ++) {
        printf("%s: ", ptr->opt_str);
        len = sizeof(val);
        if (getsockopt(fd, ptr->opt_level, ptr->opt_name, &val, &len) == -1) {
            printf("getsockopt fail.\n");
        } else {
            printf("default = %s\n", (*ptr->opt_val_str)(&val, len));
        }
    }
    return 0;
}
