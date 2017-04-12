#ifndef __TCP_CLIENT__
#define __TCP_CLIENT__

#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include "tcp_server.h"

#define SOCKET_ERROR    -1
#define SOCKET_OK	1

#define max(a,b) (a > b) ? a : b

void close_tcp_client(void);
void set_keepalive(int fd, int keep_alive, int keep_idle, int keep_interval, int keep_count);
#endif

