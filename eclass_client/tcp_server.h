#ifndef __TCP_SERVER__
#define __TCP_SERVER__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include "tcp_client.h"

#define MAXLINE     1024
#define LISTENQ     5

//#ifdef FD_SETSIZE
//#undef FD_SETSIZE
//#define FD_SETSIZE 2
//#endif

void start_tcp_server(int port,int max_listen);
void server_send_data(uint8_t* data, uint32_t size);
void close_tcp_server(void);
#endif

