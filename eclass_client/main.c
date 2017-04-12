#include "imx-decoder.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "audio_pulse.h"
#include "buildtime.h"
#include "tcp_server.h"
#include "tcp_client.h"

#define WIN_DEF_WIDTH 1368
#define WIN_DEF_HEIGHT 768
#define SIG_EXIT_FLAG 15

extern imx6_codec_t g_imx6_codec;

int debug_info = FALSE;
static void signal_process(int dunno);


struct client_arg g_client_arg;

struct tcp_server
{
	int listen_num;
	int port;
}tcpserver;

recv_thread_t g_recv_socket = {
	.sockfd = -1,
	.id = -1,
};
int need_start_tcpserver = 0;


void quit_main()
{
	printf("-======quit==============\n");
	#if 1
	//write_status_to_file("close");
	close_tcp_client();
	close_tcp_server();
	close_decoder();
	#endif
#if USE_ALSA_LIB
	audio_playback_close();
#else
	pa_playbak_reinit();
#endif
	sleep(1);
	exit(0);
}

static void signal_process(int dunno)
{
	printf("====recv signal %d \n",dunno);
	switch(dunno)
	{
		case SIG_EXIT_FLAG:
			{
				quit_main();
			}break;
		default:
			break;
	}

}

/*  check ip address */
int if_a_string_is_valid_ipv4_addr(const char *str)
{
	struct in_addr addr;
	int ret;
	errno = 0;
	ret = inet_pton(AF_INET,str,&addr);
	
	return ret;
}

/*       spliting ip to list */
void get_ip_list(char *p)
{
	int i=0,ret,n;
	char delims[]=":";
	char *result =NULL;
	result = strtok(p,delims);

	while(result != NULL){
		//ret = if_a_string_is_valid_ipv4_addr(result);
		//if(ret > 0){
			strcpy(g_client_arg.ip_list[i++],result);
		//}else{
			printf("is a invalid ip %s\n",result);
		//}

		result = strtok(NULL,delims);
	}

	g_client_arg.sum_ip_list = i;

	for(n=0;n<i;n++){
		printf("%s========%d\n",g_client_arg.ip_list[n],n);
	}

}

int option_context_parse(int argc,char *argv[])
{
	int i;

	char *ip,*port,*port1,*server_xor;
	
	ip=port=port1=server_xor=NULL;
	for(i=0;i<argc;i++){
		if (strcmp("-h",argv[i]) == 0){
			printf("%s\n",argv[i+1]);
			ip = argv[i+1];
		}
		if (strcmp("-p",argv[i]) == 0){
			printf("%s\n",argv[i+1]);
			g_client_arg.port=atoi(argv[i+1]);
			port=argv[i+1];
		}

		if (strcmp("-p1",argv[i]) == 0){
			printf("%s\n",argv[i+1]);
			need_start_tcpserver = 1;
			tcpserver.port = atoi(argv[i+1]);
		}

		if (strcmp("-s",argv[i]) == 0){
			printf("%s\n",argv[i+1]);
			server_xor = argv[i+1];
			g_client_arg.server_xor = atoi(argv[i+1]);
		}

		if (strcmp("-m",argv[i]) == 0){
			printf("%s\n",argv[i+1]);
			tcpserver.listen_num = atoi(argv[i+1]);
		}

		if (strcmp("-debug",argv[i]) == 0){
			debug_info = TRUE;
			printf("enable debug\n");
		}


	}

	printf("ip =%s,port =%s,port1=%d,xor=%s,listen_num=%d\n",ip,port,tcpserver.port,server_xor,tcpserver.listen_num);
	get_ip_list(ip);

	return need_start_tcpserver;
}

int get_debug_level(void)
{
	return debug_info;
}

#define MSDK_ALIGN16(value)             (((value/* + 15*/) >> 4) << 4)

int main(int argc,char *argv[])
{
	int flg = 0;
	int i,ret=1,need_start_tcpserver=0,loop_sum=0;

	GdkScreen *gdk_screen;
        gint screen_width, screen_height;

#if 1
	if(strstr(argv[1],"version")){
		printf("eclass_client-1.0.10-%s\n", buildtime);
		return 0;
	}
	
	if (argc < 4) {
		printf( "Usage:%s hostname portnumber server_xor\n\a", argv[0]);
		exit(1);
	}

	//signal 
	signal(SIG_EXIT_FLAG, signal_process);
	
	//save client ip
	system("ifconfig | grep eth0 -4 | grep \"inet addr\" | awk '{print $2}' > /tmp/eclass_client");

	#if USE_ALSA_LIB
	audio_playback_init();
	#else
	pa_playbak_init(argv[0]);
	#endif
	
	gtk_init(&argc, &argv);
        gdk_screen = gdk_screen_get_default();

        screen_width = gdk_screen_get_width(gdk_screen);
        screen_height= gdk_screen_get_height(gdk_screen);
        //printf("screen_width = %d\n", screen_width);
        //printf("screen_height= %d\n", screen_height);
	g_imx6_codec.width = MSDK_ALIGN16(screen_width);
	g_imx6_codec.height = MSDK_ALIGN16(screen_height);
	start_decode_process(&g_imx6_codec);
	
	need_start_tcpserver = option_context_parse(argc,argv);
	printf("need_start_tcpserver ====%d\n",need_start_tcpserver);

	if(need_start_tcpserver){
		start_tcp_server(tcpserver.port,tcpserver.listen_num);
	}
	
	g_recv_socket.portnumber = g_client_arg.port;
	g_recv_socket.server_xor = g_client_arg.server_xor;
	
	start_tcp_client();
#endif
}
