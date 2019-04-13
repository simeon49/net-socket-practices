#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"

int common_log_init(void)
{
	return 0;
}

void common_log_close(void)
{
}


int common_str_to_argv(char buf[], char ***p_argv)
{
	int num, argc;
	char **argv;
	char *p, *p0;

	common_free_argv(p_argv);

	if (buf == NULL || buf[0] == '\0')
		return -1;
	while(*buf == ' ') {
		buf ++;
		if (*buf == '\0')
			return -1;
	}
	
	num = 1;
	p0 = buf;
	while ((p = strchr(p0, ' ')) != NULL) {\
		p0 = p;
		while (*(++ p0) == ' ');
		if (*p0 == '\0')
			break;
		num ++;
	}

	if ((argv = (char **)malloc(sizeof(char *) * num)) == NULL) {
		ErrPrint("malloc %s\n", strerror(errno));
		return -1;
	}
	argc = 0;
	do {
		argv[argc] = buf;
		argc ++;
		if ((buf = strchr(buf, ' ')) == NULL)
			break;
		*buf = '\0';
		while(*(++ buf) == ' ');
	} while (*buf != '\0');
	*p_argv = argv;
	return argc;
}

void common_free_argv(char ***p_argv)
{
	if (*p_argv != NULL) {
		free(*p_argv);
	}
	*p_argv = NULL;
}
