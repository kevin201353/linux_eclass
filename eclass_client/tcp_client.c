
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include "imx-decoder.h"
#include "tcp_client.h"

  
#include <sys/ioctl.h>
#include <arpa/inet.h>   
#include <netinet/in.h> 
#include <net/if_arp.h>
#include <net/if.h>
#include <netdb.h>





extern struct client_arg g_client_arg;
extern recv_thread_t g_recv_socket;
extern int data_process(int sockfd);
static int g_sockfd;

#define SOL_TCP 6
/* Setting SO_TCP KEEPALIVE */
//int keep_alive = 1;//设定KeepAlive
//int keep_idle = 1;//开始首次KeepAlive探测前的TCP空闭时间
//int keep_interval = 1;//两次KeepAlive探测间的时间间隔
//int keep_count = 3;//判定断开前的KeepAlive探测次数
void set_keepalive(int fd, int keep_alive, int keep_idle, int keep_interval, int keep_count)
{
	int opt = 1;
	if(keep_alive){
		if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,(void*)&keep_alive, sizeof(keep_alive)) == -1){
			fprintf(stderr, "setsockopt SOL_SOCKET::SO_KEEPALIVE failed, %s\n",strerror(errno));
		}
		if(setsockopt(fd, SOL_TCP, TCP_KEEPIDLE,(void *)&keep_idle,sizeof(keep_idle)) == -1){
			fprintf(stderr,"setsockopt SOL_TCP::TCP_KEEPIDLE failed, %s\n", strerror(errno));
		}
		if(setsockopt(fd,SOL_TCP,TCP_KEEPINTVL,(void *)&keep_interval, sizeof(keep_interval)) == -1){
			fprintf(stderr,"setsockopt SOL_tcp::TCP_KEEPINTVL failed, %s\n", strerror(errno));
		}
		if(setsockopt(fd,SOL_TCP,TCP_KEEPCNT,(void *)&keep_count,sizeof(keep_count)) == -1){
			fprintf(stderr, "setsockopt SOL_TCP::TCP_KEEPCNT failed, %s\n", strerror(errno));
		}
	}
}

static void handle_connection_client(int sockfd)
{
	fd_set  rset;
	struct timeval timeout={0,0};
	
	FD_ZERO(&rset);
    	for (; ;){
		//添加连接描述符
		FD_SET(sockfd,&rset);
		//printf("--select----");fflush(stdout);
		//进行轮询
		select(sockfd+1,&rset,NULL,NULL,NULL);
		//select(sockfd+1,&rset,NULL,NULL,&timeout);
		//printf("out----");fflush(stdout);
		//测试连接套接字是否准备好
		if (FD_ISSET(sockfd, &rset)){
			if( SOCKET_ERROR ==  data_process(sockfd)){
				close(sockfd);
				printf("socket error, reconnect\n");
				fflush(stdout);
				return;
			}
		}
      
    	}
}


#if 0 //modify by linyuhua 20170302
void start_tcp_client()
{
	int i;
	int ret;
	uint32_t flag_to_server = 1;
	
	struct sockaddr_in servaddr;
	while(1){	
		for(i=0; i < g_client_arg.sum_ip_list; i++){
			
			g_sockfd = socket(AF_INET,SOCK_STREAM, 0);
			bzero(&servaddr,sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(g_recv_socket.portnumber);
			inet_pton(AF_INET, g_client_arg.ip_list[i], &servaddr.sin_addr);
			ret = connect(g_sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
			if(ret == -1){
				//fprintf(stderr, "connect to %s", g_client_arg.ip_list[i]);
				//perror(" ");
				close(g_sockfd);
				continue;
			}
			
			//fcntl(g_sockfd, F_SETFL, O_NONBLOCK);//nonblock 
			fprintf(stderr, "connect to %s ok\n", g_client_arg.ip_list[i]);
			write(g_sockfd,&flag_to_server, sizeof(uint32_t)); //sent 1 to server for getting audio data
			set_keepalive(g_sockfd, 1, 1, 1, 3);
			#if 0
			struct timeval timeout={3,0};//3s
			//int ret=setsockopt(clientfds[i],SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
			int ret=setsockopt(g_sockfd,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
			if(ret == 0){
				fprintf(stdout, "set recv socket timeout 3s\n"); fflush(stdout);	
			}
			#endif
			handle_connection_client(g_sockfd);
		}
	}
}
#else

static int get_host_ip(char *hostname, char *ip)
{
	char **pptr;
	struct hostent *hptr;
	char str[32];
	
	/* 调用gethostbyname()。调用结果都存在hptr中 */
	if( (hptr = gethostbyname(hostname) ) == NULL ){
		printf("gethostbyname error for host:%s\n", hostname);
		return FALSE; /* 如果调用gethostbyname发生错误，返回1 */
	}
	#if 0
	/* 将主机的规范名打出来 */
	printf("hostname:%s\n",hptr->h_name);
	/* 主机可能有多个别名，将所有别名分别打出来 */
	for(pptr = hptr->h_aliases; *pptr != NULL; pptr++){
		printf(" alias:%s\n",*pptr);
	}
	#endif
	/* 根据地址类型，将地址打出来 */
	switch(hptr->h_addrtype){
		case AF_INET:
		case AF_INET6:
			pptr=hptr->h_addr_list;
			/* 将刚才得到的所有地址都打出来。其中调用了inet_ntop()函数 */
			for(;*pptr!=NULL;pptr++){
				//printf(" address:%s\n", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
				sprintf(ip, "%s\n", inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
			}

			break;
		default:
			printf("unknown address type\n");
			break;
	}
	return TRUE;

}

void start_tcp_client()
{
	int i;
	int ret;
	uint32_t flag_to_server = 1;
	char ip[32];
	
	struct sockaddr_in servaddr;
	while(1){	
		for(i=0; i < g_client_arg.sum_ip_list; i++){
			if(inet_aton(&g_client_arg.ip_list[i], &(servaddr.sin_addr)) == 0) {
				memset(ip, 0x0, 32);
				if(TRUE == get_host_ip(g_client_arg.ip_list[i], &ip)){
					printf("hostname:%s ip:%s\n", g_client_arg.ip_list[i], ip);
					if(inet_aton(ip, &(servaddr.sin_addr)) == 0){
						fprintf(stderr, "the hostip is not right");
						return FALSE; 
					}
				}else{
					printf("server:%s\n", g_client_arg.ip_list[i]);
					fprintf(stderr, "-------can not get host ip---------");
					return FALSE; 
				}
			}
			
			g_sockfd = socket(AF_INET,SOCK_STREAM, 0);
			//bzero(&servaddr,sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(g_recv_socket.portnumber);
			//inet_pton(AF_INET, &ip, &servaddr.sin_addr);
			ret = connect(g_sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
			if(ret == -1){
				//fprintf(stderr, "connect to %s", g_client_arg.ip_list[i]);
				//perror(" ");
				close(g_sockfd);
				continue;
			}
			
			//fcntl(g_sockfd, F_SETFL, O_NONBLOCK);//nonblock 
			fprintf(stderr, "connect to %s ok\n", g_client_arg.ip_list[i]);
			//zhaosenhua update
			char szTmp[20] = {0};
			sprintf(szTmp, "%d", flag_to_server);
			write(g_sockfd, szTmp, sizeof(szTmp));
			//update end
			
			//write(g_sockfd,&flag_to_server, sizeof(uint32_t)); //sent 1 to server for getting audio data
			set_keepalive(g_sockfd, 1, 1, 1, 3);
			#if 0
			struct timeval timeout={3,0};//3s
			//int ret=setsockopt(clientfds[i],SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
			int ret=setsockopt(g_sockfd,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
			if(ret == 0){
				fprintf(stdout, "set recv socket timeout 3s\n"); fflush(stdout);	
			}
			#endif
			handle_connection_client(g_sockfd);
		}
	}
}
#endif

void close_tcp_client(void)
{
	close(g_sockfd);
}

