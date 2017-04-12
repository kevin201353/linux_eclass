/*
#if 0
IO多路复用之select总结

1、基本概念

　　IO多路复用是指内核一旦发现进程指定的一个或者多个IO条件准备读取，它就通知该进程。IO多路复用适用如下场合：

　　（1）当客户处理多个描述字时（一般是交互式输入和网络套接口），必须使用I/O复用。

　　（2）当一个客户同时处理多个套接口时，而这种情况是可能的，但很少出现。

　　（3）如果一个TCP服务器既要处理监听套接口，又要处理已连接套接口，一般也要用到I/O复用。

　　（4）如果一个服务器即要处理TCP，又要处理UDP，一般要使用I/O复用。

　　（5）如果一个服务器要处理多个服务或多个协议，一般要使用I/O复用。

　　与多进程和多线程技术相比，I/O多路复用技术的最大优势是系统开销小，系统不必创建进程/线程，也不必维护这些进程/线程，从而大大减小了系统的开销。

2、select函数

　　该函数准许进程指示内核等待多个事件中的任何一个发送，并只在有一个或多个事件发生或经历一段指定的时间后才唤醒。函数原型如下：

#include <sys/select.h>
#include <sys/time.h>

int select(int maxfdp1,fd_set *readset,fd_set *writeset,fd_set *exceptset,const struct timeval *timeout)
返回值：就绪描述符的数目，超时返回0，出错返回-1
函数参数介绍如下：

（1）第一个参数maxfdp1指定待测试的描述字个数，它的值是待测试的最大描述字加1（因此把该参数命名为maxfdp1），描述字0、1、2...maxfdp1-1均将被测试。

（2）中间的三个参数readset、writeset和exceptset指定我们要让内核测试读、写和异常条件的描述字。如果对某一个的条件不感兴趣，就可以把它设为空指针。struct fd_set可以理解为一个集合，这个集合中存放的是文件描述符，可通过以下四个宏进行设置：

          void FD_ZERO(fd_set *fdset);           //清空集合

          void FD_SET(int fd, fd_set *fdset);   //将一个给定的文件描述符加入集合之中

          void FD_CLR(int fd, fd_set *fdset);   //将一个给定的文件描述符从集合中删除

          int FD_ISSET(int fd, fd_set *fdset);   // 检查集合中指定的文件描述符是否可以读写 

（3）timeout告知内核等待所指定描述字中的任何一个就绪可花多少时间。其timeval结构用于指定这段时间的秒数和微秒数。

         struct timeval{

                   long tv_sec;   //seconds

                   long tv_usec;  //microseconds

       };

这个参数有三种可能：

（1）永远等待下去：仅在有一个描述字准备好I/O时才返回。为此，把该参数设置为空指针NULL。

（2）等待一段固定时间：在有一个描述字准备好I/O时返回，但是不超过由该参数所指向的timeval结构中指定的秒数和微秒数。

（3）根本不等待：检查描述字后立即返回，这称为轮询。为此，该参数必须指向一个timeval结构，而且其中的定时器值必须为0。

3、测试程序

　　写一个TCP回射程序，程序的功能是：客户端向服务器发送信息，服务器接收并原样发送给客户端，客户端显示出接收到的信息。

服务端程序如下所示：
#endif
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <linux/if.h>
#include <fcntl.h>
#include "tcp_server.h"
#include "pthread.h"
//#include "tcp_client.h"

extern int send_data(int fd, void* data, uint32_t size);

//函数声明
//创建套接字并进行绑定
static int socket_bind(const char* ip,int port);
//IO多路复用select
static void do_select(int listenfd);
//处理多个连接
static void handle_connection_server(int *connfds,int num,fd_set *prset, fd_set *pallset);

static int g_max_client = 0;
static int g_server_port = 0;
static int clientfds[FD_SETSIZE];	//保存客户连接描述符
static int maxi;			//记录客户连接套接字的个数
static fd_set rset;
static fd_set allset;
static int  listenfd;

int getlocalhostip(char *ip)
{
        int sockfd;
        if(-1 == (sockfd = socket(PF_INET, SOCK_STREAM, 0))){
                perror( "socket" );
                return -1;
        }
        
        struct ifreq req;
        struct sockaddr_in *host;
        bzero(&req, sizeof(struct ifreq));
        strcpy(req.ifr_name, "eth0");
        ioctl(sockfd, SIOCGIFADDR, &req);
        host = (struct sockaddr_in*)&req.ifr_addr;
        strcpy(ip, inet_ntoa(host->sin_addr));
        close(sockfd);
        return 1;

}

int tcp_server_thread(void* args)
{
	char ipbuf[16];

	memset(ipbuf, 0, 16);
	getlocalhostip((char*)&ipbuf);
	printf("%s getlocalhostip=%s\n", __FUNCTION__, ipbuf);
	listenfd = socket_bind(ipbuf, g_server_port);
	listen(listenfd,LISTENQ);
	do_select(listenfd);
	return 0;
}

static int socket_bind(const char* ip,int port)
{
	int  listenfd;
	int  on = 1;
	struct sockaddr_in servaddr;

	listenfd = socket(AF_INET,SOCK_STREAM,0);
	if (listenfd == -1){
		perror("socket error:");
		exit(1);
	}
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET,ip,&servaddr.sin_addr);
	servaddr.sin_port = htons(port);
	setsockopt(listenfd, SOL_SOCKET,SO_REUSEADDR, &on, sizeof(on));
	//fcntl(listenfd, F_SETFL, O_NONBLOCK);//nonblock 
	if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1){
		perror("bind error: ");
		exit(1);
	}
	return listenfd;
}


static void handle_connection_server(int *connfds,int num,fd_set *prset,fd_set *pallset)
{
	int i,n;
	char buf[MAXLINE];
	
	memset(buf,0,MAXLINE);
	for (i = 0;i <= num;i++){
		if (connfds[i] < 0)
			continue;
			
		//测试客户描述符是否准备好
		if (FD_ISSET(connfds[i],prset)){
			//接收客户端发送的信息
			n = read(connfds[i],buf,MAXLINE);
			if (n == 0){
				printf("close client socket\n");
				fflush(stdout);
				close(connfds[i]);
				FD_CLR(connfds[i],pallset);
				connfds[i] = -1;
				continue;
			}
		}
	}
}

void server_send_data(uint8_t* data, uint32_t size)
{
	int i;
	for (i = 0;i <= maxi; i++){
		if (clientfds[i] < 0)
			continue;
			
		//测试客户描述符是否准备好
		if (FD_ISSET(clientfds[i], &rset)){
			//向客户端发送buf
			if(SOCKET_ERROR == send_data(clientfds[i], data, size)){
				printf("write-----close client socket\n");
				fflush(stdout);
				close(clientfds[i]);
				FD_CLR(clientfds[i], &allset);
				clientfds[i] = -1;
			}
		}
	}
}

static void do_select(int listenfd)
{
	int  connfd,sockfd;
	struct sockaddr_in cliaddr;
	socklen_t cliaddrlen;
	int maxfd;
	int i;
	int nready;
	//初始化客户连接描述符
	for (i = 0;i < FD_SETSIZE;i++)
		clientfds[i] = -1;
	maxi = -1;
	FD_ZERO(&allset);
	//添加监听描述符
	FD_SET(listenfd,&allset);
	maxfd = listenfd;
	//循环处理
	for ( ; ; ){
		rset = allset;
		//获取可用描述符的个数
		nready = select(maxfd+1,&rset,NULL,NULL,NULL);
		if (nready == -1){
			perror("select error:");
			exit(1);
		}
		//测试监听描述符是否准备好
		if (FD_ISSET(listenfd,&rset)){
			cliaddrlen = sizeof(cliaddr);
			//接受新的连接
			if ((connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen)) == -1){
				if (errno == EINTR)
					continue;
				else{
					perror("accept error:");
					exit(1);
				}
			}
			fprintf(stdout,"accept a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
			fflush(stdout);

			//将新的连接描述符添加到数组中
			for (i = 0;i < g_max_client; i++){
				if (clientfds[i] < 0){
					clientfds[i] = connfd;
					//fcntl(clientfds[i], F_SETFL, O_NONBLOCK);//nonblock 
					//set_keepalive(clientfds[i], 1, 1, 1, 3);
					#if 1
					struct timeval timeout={1,0};//1s
					int ret=setsockopt(clientfds[i],SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
					//int ret=setsockopt(sock_fd,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
					if(ret == 0){
						fprintf(stdout,"set socket timeout 1s \n");
						fflush(stdout);	
					}
					#endif
					//write(connfd,"open", 4);
					break;
				}
			}
			if (i == g_max_client){
				fprintf(stderr,"too many clients. close it\n");
				fflush(stdout);
				//write(connfd,"close", 5);
				close(connfd);
				continue;
				//exit(1);
			}
			//将新的描述符添加到读描述符集合中
			FD_SET(connfd, &allset);
			//描述符个数
			maxfd = (connfd > maxfd ? connfd : maxfd);
			
			//记录客户连接套接字的个数
			maxi = (i > maxi ? i : maxi);
			if (--nready <= 0)
				continue;
		}
		//处理客户连接
		handle_connection_server(clientfds, maxi, &rset, &allset);
	}
}


void start_tcp_server(int port,int max_listen)
{
	int ret = 0;
	
	g_server_port = port;
	g_max_client = max_listen;

	
	pthread_t thread_id;
	
	ret = pthread_create(&thread_id, NULL,tcp_server_thread, NULL);
	if(ret < 0){
		perror("create pthread error");
		exit(1);
	}
	pthread_detach(thread_id);
}

void close_tcp_server()
{
	#if 1
	int i;
	for (i = 0;i <= maxi; i++){
		if (clientfds[i] < 0)
			continue;
		close(clientfds[i]);
		FD_CLR(clientfds[i],&allset);
		clientfds[i] = -1;
	}
	#endif
	close(listenfd);
}

