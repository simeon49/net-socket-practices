#include <unistd.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <netpacket/packet.h>
#include <netinet/ip.h>
#include "string.h"
#define SIZE 8192

int set_promisc(char* interface, int sock);  //将网卡设置为混杂模式
int unset_promisc(char* interface, int fd); //将网卡的混杂模式取消
int str_int(char* str);//将字符串转化为数字

int main(int argc,char** argv)
{
 int sock;
 char buf[SIZE]; //接受数据缓冲区
 int data_len;    // 接受数据的长度
 int data_num; //监听数据包的个数
 int i;
 FILE *fp1;   //将接受数据存储在fp1指针所指的文件中
 FILE *fp2;   //将接受数据的长度存储在fp2指针所指的文件中


 //确定需要监听数据包的个数

 if(argc!=2)
 {
  printf("arguments is not enough!\n");
  return -1;
 }
 data_num = 0;
 data_num = str_int(argv[1]);
 if(data_num<1)
 {
  return -1;
 }


 //以写方式打开file1.txt和file2.txt
 if((fp1=fopen("file1.txt","w"))==NULL)
 {
  printf("error in open file!\n");
  return 0;

 }
 if((fp2=fopen("file2.txt","w"))==NULL)
 {
  printf("error in open file!\n");
  return 0;

 }

//初始化变量
 memset(buf,0,sizeof(buf));
 //memset(straddr,0,sizeof(straddr));
 data_len=0;

//PF_PACKET 与SOCKET_RAW配合表示接受完整的以太网数据帧,

//AF_INET 与SOCKET_RAW配合表示接受完整的IP据帧

//htons()将主机的无符号短整形数转换成网络字节顺序 简单地说,htons()就是将一个数的高低位互换

  if((sock = socket(PF_PACKET, SOCK_RAW,htons(ETH_P_IP)))==-1)
 {
  printf("create SOCK_package Socket failed.\n");
  return 0;
 }

//将网卡wlan0设置为混杂模式
  if(!set_promisc("wlan0" , sock))
 {
  printf("error in set_promisc\n");
  return 0;
 }

//接受数据并将数据存储在文件中
 for(i=0;i<data_num;i++)
 {
  if((data_len=recvfrom(sock,buf,SIZE,0,NULL,0))<1)
  {
   printf("error in recv!-------\n");
   return 0;
  }
  printf("--");

  fwrite(buf,data_len,1,fp1);
  fprintf(fp2,"%d ",data_len);
 }
 printf("\n");

//将网卡混合模式取消
 if(unset_promisc("wlan0",sock)==-1)
 {
  printf("error in unsetpromisc!");
 }

//关闭套接字、文件。
 close(sock);
 fclose(fp1);
 fclose(fp2);
 return 0;
}






int set_promisc(char* interface, int sock)
{
 struct ifreq ifr;
 strcpy(ifr.ifr_name,interface);
 //SIOCGIFFLAGS  is define in sys/ioctl.h
 //read flag of the interface
 if(ioctl(sock, SIOCGIFFLAGS , &ifr) == -1)
 {
  printf("could not recive flag for the interface.\n");
  return 0;
 }
 //set flga of the interface to promisc
 ifr.ifr_flags |= IFF_PROMISC;
 if(ioctl(sock, SIOCSIFFLAGS , &ifr) == -1)
 {
  printf("could not set flag.\n");
  return 0;
 }
 printf("Setting interface ::: %s ::: to promisc\n", interface);
 return 1;
}
int unset_promisc(char *interface,int fd)
{
 struct ifreq ifr;
 strcpy(ifr.ifr_name,interface);
 if(ioctl(fd,SIOCGIFFLAGS,&ifr)==-1)
 {
  printf("error in get ioctl!\n");
  return -1;
 }
 ifr.ifr_flags&=~IFF_PROMISC;
 if(ioctl(fd,SIOCSIFFLAGS,&ifr)==-1)
 {
  printf("error in set ioctl!\n");
  return -1;
 }
 return 0;
}
int str_int(char* str)
{
 char* buf;
 int t;
 int i;
 int n;

 buf = str;
 t = 0;
 n = 0;
 for(i = 0;buf[i];i++)
 {
  t = buf[i]-'0';
  if(t>9||t<0)
  {
   printf("the input shoudbe a number!\n");
   return -2;
  }
  n = n*10 + t;
 }
 return n;

}
