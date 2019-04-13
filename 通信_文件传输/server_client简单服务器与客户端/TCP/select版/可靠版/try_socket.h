/*
	try_socket
*/
 
extern int try_socket(int domain, int type, int protocol);

extern int try_bind(int sockfd, const struct sockaddr_in *addr);

extern int try_listen(int sockfd, int backlog);

extern int try_connect(int sockfd, struct sockaddr_in *p_address);


#define TRY_SEND_TIMEOUT 3
#define TRY_RECV_TIMEOUT 5

#define SEND_RETRY_TIME 10000	/*10ms*/
extern int try_send(int sockfd, const void  *buf, size_t len, unsigned int timeout);

extern int try_recv(int sockfd, void *buf, size_t len, unsigned int timeout);


