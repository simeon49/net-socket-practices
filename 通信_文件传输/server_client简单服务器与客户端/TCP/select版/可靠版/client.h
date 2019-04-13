#include <cutils/properties.h>
#include "system_properties.h"
#include "try_socket.h"
#include "common.h"


#define RESULT_KEY "thakral.result"

#define ERR_PROGRAM_ERROR "ERROR \"program error try again\""
#define ERR_NET_ERROR "ERROR \"network error\""

/*
	buf 命令格式：[IP地址] [端口] [收件邮箱] [命令] [附加参数1]..[附加参数n]
		"192.168.1.212 8900 email1@163.com SETPARAM thakral.send.email.a=1234@163.com thakral.send.email.p=123"

		端口 8900
		命令：
 	GETPARAM :获取参数值 
		buf=“192.168.1.212 8900 email1@163.com GETPARAM thakral.send.email.p thakral.alarmmotion.recordflag”
		函数成功时返回 0 "thakral.result"被设置为 “PARAM thakral.send.email.p=1234 thakral.alarmmotion.recordflag=1”
	SETPARAM :设置参数值
		buf=“192.168.1.212 8900 email1@163.com SETPARAM thakral.alarmmotion.recordflag=0”
		函数成功时返回 0 "thakral.result"被设置为 “OK thakral.alarmmotion.recordflag=0”
	GETMSG :获取报警消息
		buf=“192.168.1.212 8900 email1@163.com GETMSG”
		函数成功时返回 0 "thakral.result"被设置为 “MSG 93 2013/08/14_10:14:46 Alarm 130814/1013291.mp4” 或 “MSG 0”
			其中93，0为消息编号 为0是表示没有新消息
	如果函数返回-1 表示发生错误 "thakral.result"被设置为 “ERROR “*****产生错误原因*****””
	
		
*/
extern int client(const char *buf);

