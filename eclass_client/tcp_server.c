/*
#if 0
IO��·����֮select�ܽ�

1����������

����IO��·������ָ�ں�һ�����ֽ���ָ����һ�����߶��IO����׼����ȡ������֪ͨ�ý��̡�IO��·�����������³��ϣ�

������1�����ͻ�������������ʱ��һ���ǽ���ʽ����������׽ӿڣ�������ʹ��I/O���á�

������2����һ���ͻ�ͬʱ�������׽ӿ�ʱ������������ǿ��ܵģ������ٳ��֡�

������3�����һ��TCP��������Ҫ��������׽ӿڣ���Ҫ�����������׽ӿڣ�һ��ҲҪ�õ�I/O���á�

������4�����һ����������Ҫ����TCP����Ҫ����UDP��һ��Ҫʹ��I/O���á�

������5�����һ��������Ҫ�������������Э�飬һ��Ҫʹ��I/O���á�

���������̺Ͷ��̼߳�����ȣ�I/O��·���ü��������������ϵͳ����С��ϵͳ���ش�������/�̣߳�Ҳ����ά����Щ����/�̣߳��Ӷ�����С��ϵͳ�Ŀ�����

2��select����

�����ú���׼�����ָʾ�ں˵ȴ�����¼��е��κ�һ�����ͣ���ֻ����һ�������¼���������һ��ָ����ʱ���Ż��ѡ�����ԭ�����£�

#include <sys/select.h>
#include <sys/time.h>

int select(int maxfdp1,fd_set *readset,fd_set *writeset,fd_set *exceptset,const struct timeval *timeout)
����ֵ����������������Ŀ����ʱ����0��������-1
���������������£�

��1����һ������maxfdp1ָ�������Ե������ָ���������ֵ�Ǵ����Ե���������ּ�1����˰Ѹò�������Ϊmaxfdp1����������0��1��2...maxfdp1-1���������ԡ�

��2���м����������readset��writeset��exceptsetָ������Ҫ���ں˲��Զ���д���쳣�����������֡������ĳһ��������������Ȥ���Ϳ��԰�����Ϊ��ָ�롣struct fd_set�������Ϊһ�����ϣ���������д�ŵ����ļ�����������ͨ�������ĸ���������ã�

          void FD_ZERO(fd_set *fdset);           //��ռ���

          void FD_SET(int fd, fd_set *fdset);   //��һ���������ļ����������뼯��֮��

          void FD_CLR(int fd, fd_set *fdset);   //��һ���������ļ��������Ӽ�����ɾ��

          int FD_ISSET(int fd, fd_set *fdset);   // ��鼯����ָ�����ļ��������Ƿ���Զ�д 

��3��timeout��֪�ں˵ȴ���ָ���������е��κ�һ�������ɻ�����ʱ�䡣��timeval�ṹ����ָ�����ʱ���������΢������

         struct timeval{

                   long tv_sec;   //seconds

                   long tv_usec;  //microseconds

       };

������������ֿ��ܣ�

��1����Զ�ȴ���ȥ��������һ��������׼����I/Oʱ�ŷ��ء�Ϊ�ˣ��Ѹò�������Ϊ��ָ��NULL��

��2���ȴ�һ�ι̶�ʱ�䣺����һ��������׼����I/Oʱ���أ����ǲ������ɸò�����ָ���timeval�ṹ��ָ����������΢������

��3���������ȴ�����������ֺ��������أ����Ϊ��ѯ��Ϊ�ˣ��ò�������ָ��һ��timeval�ṹ���������еĶ�ʱ��ֵ����Ϊ0��

3�����Գ���

����дһ��TCP������򣬳���Ĺ����ǣ��ͻ����������������Ϣ�����������ղ�ԭ�����͸��ͻ��ˣ��ͻ�����ʾ�����յ�����Ϣ��

����˳���������ʾ��
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

//��������
//�����׽��ֲ����а�
static int socket_bind(const char* ip,int port);
//IO��·����select
static void do_select(int listenfd);
//����������
static void handle_connection_server(int *connfds,int num,fd_set *prset, fd_set *pallset);

static int g_max_client = 0;
static int g_server_port = 0;
static int clientfds[FD_SETSIZE];	//����ͻ�����������
static int maxi;			//��¼�ͻ������׽��ֵĸ���
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
			
		//���Կͻ��������Ƿ�׼����
		if (FD_ISSET(connfds[i],prset)){
			//���տͻ��˷��͵���Ϣ
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
			
		//���Կͻ��������Ƿ�׼����
		if (FD_ISSET(clientfds[i], &rset)){
			//��ͻ��˷���buf
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
	//��ʼ���ͻ�����������
	for (i = 0;i < FD_SETSIZE;i++)
		clientfds[i] = -1;
	maxi = -1;
	FD_ZERO(&allset);
	//��Ӽ���������
	FD_SET(listenfd,&allset);
	maxfd = listenfd;
	//ѭ������
	for ( ; ; ){
		rset = allset;
		//��ȡ�����������ĸ���
		nready = select(maxfd+1,&rset,NULL,NULL,NULL);
		if (nready == -1){
			perror("select error:");
			exit(1);
		}
		//���Լ����������Ƿ�׼����
		if (FD_ISSET(listenfd,&rset)){
			cliaddrlen = sizeof(cliaddr);
			//�����µ�����
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

			//���µ�������������ӵ�������
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
			//���µ���������ӵ���������������
			FD_SET(connfd, &allset);
			//����������
			maxfd = (connfd > maxfd ? connfd : maxfd);
			
			//��¼�ͻ������׽��ֵĸ���
			maxi = (i > maxi ? i : maxi);
			if (--nready <= 0)
				continue;
		}
		//����ͻ�����
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

