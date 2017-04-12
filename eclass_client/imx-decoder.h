#ifndef __MEDIA_HEADER__
#define __MEDIA_HEADER__

#include <stdint.h>
#include <sys/types.h>   
#include <sys/socket.h>   
#include <arpa/inet.h>   
#include <netinet/in.h>   
#include <strings.h>   
#include <errno.h>   
#include <gst/gst.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
//#include <gst/interfaces/xoverlay.h>	//gstreamer-0.10
#include <gst/video/videooverlay.h>	//gstreamer-1.0
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <error.h>
//#include <fcntl.h>
#include "/usr/include/asm-generic/fcntl.h"
#include <stdint.h>
#include <sys/time.h>

#define STREAM_TYPE_MJPEG	1
#define STREAM_TEPY_H264	2

#define FRAME_TYPE_MJPEG        0
#define FRAME_TYPE_H264_I       1
#define FRAME_TYPE_H264_P       2
#define FRAME_TYPE_AUDIO        3
#define FRAME_TYPE_CURSOR	4

#define WAIT_DATA_TIMEOUT	3000 //ms

#define DATA_FIFO "/tmp/eclassdatafifo"
#define TOTAL_DATA_LEN 0x100000 // 1m
#define DUMMY_DATA_LEN TOTAL_DATA_LEN  // 1m
#define SECURE_CHECK	0x40501120

extern int get_debug_level(void);

#define LOG_DEBUG(format, ...)						\
	do {													\
		struct timespec __now;								\
															\
		if (!clock_gettime(CLOCK_REALTIME, &__now)) {		\
			printf(" [%ld:%06ld] DEBUG [%s]: " format "\n",	\
				__now.tv_sec, __now.tv_nsec / 1000,			\
				__func__, ## __VA_ARGS__); fflush(stdout);	\
		} else {											\
			printf("DEBUG [%s]: " format "\n",				\
				__func__, ## __VA_ARGS__); fflush(stdout);	\
		}													\
	} while (0)


#define eclass_client_debug(format, ...)	\
	do {					\
		if (get_debug_level() == TRUE){	\
			LOG_DEBUG(format, ## __VA_ARGS__);	\
		} \
	} while (0)
	
	
extern GtkWidget *main_window;
extern GtkWidget *video_output;
extern int debug_info;

typedef struct
{
    GMainLoop *loop;
    GstElement *pipeline;
    GstElement *source;
    GstElement *parse;
    GstElement *decoder;
    GstElement *sink;
    pthread_mutex_t mutex;
    pthread_t thread_id;
    int frame_count;
    int jpeg_w, jpeg_h;
    int disp_x, disp_y, disp_w, disp_h;
    struct timeval tv0;
    struct timeval tv1;
    int  type;
    int len;
}vpudec_data_t;

struct client_arg
{
	char ip_list[30][30];
	int port;
	int server_xor;
	int sum_ip_list;
};

enum thread_state { none=0, starting, running, stopped };
typedef struct
{
        int fifo_fd;
        int type;
        enum thread_state tstate;
        int width;
        int height;
}imx6_codec_t;

typedef struct{
	int sockfd;
	pthread_t id;
	pthread_mutex_t mutex;
	struct sockaddr_in server_addr;
	int portnumber;
	int server_xor;
	
}recv_thread_t;
//typedef struct
//{
//        int type;
//        int width;
//        int height;
//        int init_flg;
//}current_imx_dec_info;

struct media_header
{
	char magic[4];
	uint32_t type; // 0：MJPEG 1: H264 I frame, 2: H264 P frame, 3: Audio frame
	uint32_t serial; // audio/video id
	uint16_t width; // frame width
	uint16_t height; // frame height
	uint16_t cursor_x; // Mouse pointer X
	uint16_t cursor_y; //  Mouse pointer Y
	uint32_t time_stamp; // 单位：毫秒（ms）
	uint32_t size; // 后续数据长度，不包含此header长度
	uint32_t checksum;
}__attribute__((packed));

void imx_decoder_process(int width, int height);
void stop_imx_decoder_process(void);
long get_current_time_ms(void);

#endif
