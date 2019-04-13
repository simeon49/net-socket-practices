/*
*	common file
*/

#include <string.h>
#include <pthread.h>
#include <errno.h>


extern int common_log_init(void);

#define ErrPrint(...)	do{fprintf(stderr, "Error in function %s(): ", __func__);fprintf(stderr, __VA_ARGS__);}while(0)
#define Print(...)	do{fprintf(stderr, __VA_ARGS__);}while(0)
//#define Print(...)

extern void common_log_close(void);

/*	server.c client.c socket接收 发送缓存区的大小*/
#define SEND_BUFSIZE 20480
#define RECV_BUFSIZE 20480


/*
*	以空格为分割符 切割buf字符串 将切割结果保存在*p_argv中 
*	USAGE:	int my_argc
			char **my_argv = NULL;
			char buf[100] = "    GE      HIo IOHOIH      fasdf              ";

			if ((my_argc = common_str_to_argv(buf, &my_argv)) > 0) {
				for (i = 0; i < my_argc; i ++) {
					printf("[%s]", my_argv[i]);
			}
			printf("\n");
			common_free_argv (&myargv);
*	NOTICE:	调用该函数将会改变buf本身  调用free()释放
*	RETURN: 子字符串个数(不含空格)(success)  -1(error)
*/
extern int common_str_to_argv(char buf[], char ***p_argv);

extern void common_free_argv(char ***p_argv);
