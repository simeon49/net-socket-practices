/* ************************************************************************
 *       Filename:  socket_headfile.h
 *    Description:
 *        Version:  1.0
 *        Created:  2013年02月14日
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (),
 *        Company:
 * ************************************************************************/
#ifndef _SOCKET_HEADFILE_
#define _SOCKET_HEADFILE_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define errexit(_errmsg) do{fprintf(stderr, "error: [%s] %s\n", _errmsg, strerror(errno)); exit(EXIT_FAILURE);}while(0)		//使用时小心：例如 if(.....) errexit("xx"); 错误
														//if(....){errexit("xx");} 正确
#define	SEVERIP "192.168.0.100"				//服务器IP
#define SEVERPORT 7979						//服务器端口

#endif
