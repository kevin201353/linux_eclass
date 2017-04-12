#include "imx-decoder.h"
#include "audio_pulse.h"
#include <dirent.h>
#include "tcp_server.h"
#include "tcp_client.h"
#include <stdio.h>

extern int start_imx6_decoder(imx6_codec_t* pcodec);
extern imx6_codec_t g_imx6_codec;

extern int imx_decoder_error_flg;
extern int imx_decoder_error_end;
extern int wait_h264_i_frame_flg;
extern recv_thread_t g_recv_socket;
extern int need_start_tcpserver;

static uint8_t data_buf[TOTAL_DATA_LEN];

long get_current_time_ms()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


int write_status_to_file(char *status)
{
	/*
	FILE *fp;
	printf("=============%s\n",status);

	if(access("/tmp/syplayer",F_OK) < 0)
	{
		system("mkdir /tmp/syplayer");
	}
	
	fp = fopen("/tmp/syplayer/eclass_state","w");
	if(fp < 0)
		{
			perror("cant no open file");
			return ;
		}

	fprintf(fp,"%s\n",status);

	fclose(fp);
	*/
	return 0;

}



static int should_drop_data(struct media_header* header)
{
	if(FRAME_TYPE_MJPEG == header->type){
		printf("%s %d\n", __FUNCTION__, __LINE__);
		return FALSE;
	}
	if(FRAME_TYPE_H264_I == header->type){
		//printf("%s %d\n", __FUNCTION__, __LINE__);
		wait_h264_i_frame_flg = FALSE;
		return FALSE;
	}
	if(TRUE == wait_h264_i_frame_flg){
		printf("drop data, waiting for H264-I frame\n");
		return TRUE;    
	}else{
		return FALSE;
	}
}

int recv_data(int sock_fd, void *buffer, int len)
{

	int l, ret;
	void *d;
	static int err_count=0;

	d = buffer;
	l = len;

	while(l){
		ret = read(sock_fd, d, l);
		#if 0	
		if(ret ==-1 && errno==EAGAIN){
			printf("recv timeout\n");
		}
		#endif
		if( ret <= 0 ){
			perror("socket read");
			return SOCKET_ERROR;
		}
		l -= ret;
		d += ret;
		//printf("recv_data l :%d.\n", l);
	}
	return SOCKET_OK;
}


int send_data(int fd, void* data, uint32_t size)
{
	int l;
	int ret;
	char *d;
	 
	d = data;
	l = size;
	while(l){
	        /* 开始写*/
	        ret  = write(fd, data, l);
	        if(ret <= 0){
	        	perror("socket write");
	        	if(errno == EINTR){ /* 中断错误 我们继续写*/
	                        continue;
	                }else{             /* 其他错误 没有办法,只好撤退了*/
	                        return SOCKET_ERROR;
	                }
	        }
	        l -= ret;
	        d += ret;
	}
	return SOCKET_OK;
}
static void dump_data(uint32_t type, uint8_t* data, int data_size)
{
	char file_str[200];
	static uint32_t id;

	DIR *dir=NULL;
	dir = opendir("/tmp/spice_dump");
	if(NULL == dir){
		printf("mkdir -p /tmp/spice_dump\n");
		system("mkdir -p /tmp/spice_dump");
	}else{
		closedir(dir);
	}

	++id;
	if(id == 1){
		system("sudo rm -f /tmp/spice_dump/*");
	}

	if(id > 100){
		sprintf(file_str, "/tmp/spice_dump/dump.%d",id-100);
		remove(file_str);
	}
	sprintf(file_str, "/tmp/spice_dump/dump.%d",id);
	if(id==1 || (id%100 == 0)){
		printf("------dump data %s----------\n", file_str);
		fflush(stdout);
	}
	FILE *f = fopen(file_str, "wb");
	if (!f) {
		printf("open file error \n");
		return;
	}

	fwrite(data, 1, data_size, f);
	fclose(f);
}

int data_process(int sockfd)
{
	int ret = 0;
	int i;
	char* magic = "BEAF";
	uint32_t *data;
	uint32_t checksum;
	int fend  = 0x45464353; //SCFE
	
	struct media_header media_header;
	if(SOCKET_ERROR == recv_data(sockfd, &media_header, sizeof(media_header))){
		return SOCKET_ERROR;
	}
	#if 0
	//test  zhaosenhua update
	for( i=0; i<4; i++ ){
		printf("%c\n", media_header.magic[i]);fflush(stdout);
	}
	printf("media_header.type=%d\n", media_header.type);fflush(stdout);
	printf("media_header.serial=%d\n", media_header.serial);fflush(stdout);
	printf("media_header.width=%d\n", media_header.width);fflush(stdout);
	printf("media_header.height=%d\n", media_header.height);fflush(stdout);
	printf("media_header.size=%d\n", media_header.size);fflush(stdout);
	printf("media_header.checksum=0x%x\n", media_header.checksum);fflush(stdout);
	//printf("my test :%s.\n", "111000");fflush(stdout);
	//teset end
	for( i=0; i<4; i++ ){
		if(media_header.magic[i] != *(magic+i)){
			printf("media_header.magic[%d]=%d\n", i, media_header.magic[i]);fflush(stdout);
			return SOCKET_OK;	
		}
	}
	#endif
	//printf("my test :%s.\n", "111111");fflush(stdout);
	if(need_start_tcpserver == 1){
		server_send_data((uint8_t *)&media_header, sizeof(media_header));
	}
	//printf("my test :%s.\n", "22222");fflush(stdout);

#if 0
	for( i=0; i<4; i++ ){
		printf("%c\n", media_header.magic[i]);fflush(stdout);
	}
	printf("media_header.type=%d\n", media_header.type);fflush(stdout);
	printf("media_header.serial=%d\n", media_header.serial);fflush(stdout);
	printf("media_header.width=%d\n", media_header.width);fflush(stdout);
	printf("media_header.height=%d\n", media_header.height);fflush(stdout);
	printf("media_header.size=%d\n", media_header.size);fflush(stdout);
	printf("media_header.checksum=0x%x\n", media_header.checksum);fflush(stdout);

#else
	//printf("media_header.size=%d\n", media_header.size);fflush(stdout);
#endif
	memset(data_buf, 0, TOTAL_DATA_LEN);
	if(SOCKET_ERROR == recv_data(sockfd, data_buf, media_header.size)){
		return SOCKET_ERROR;
	}
	#if 0
	FILE * stream;
	stream = fopen("/tmp/fwrite", "a+");
	fwrite(data_buf, 1, media_header.size, stream);
	fclose(stream);
	#endif
    //test zhaosenhua
	printf("recv qt tcpserver data len: %d.\n", media_header.size);fflush(stdout);
	//test end
	if(need_start_tcpserver == 1){
		server_send_data(data_buf, media_header.size);
	}
		
	if(media_header.type == FRAME_TYPE_CURSOR){
		//to do cursor_process
		return SOCKET_OK;
	}
	
	data = (uint32_t*)data_buf;
	checksum = 0;
	for( i=0; i < media_header.size/4; i++){
		checksum += data[i];
		data[i] ^= g_recv_socket.server_xor;
	}
		
	if(checksum != media_header.checksum){
		printf("checksum error, checksum value: %d.\n", media_header.checksum);fflush(stdout);
		return SOCKET_OK;
	}

	if(media_header.type == FRAME_TYPE_AUDIO){
		#if USE_ALSA_LIB
		audio_playback_data(data_buf, media_header.size);
		#else
		pa_playbak_playing(data_buf, media_header.size);
		#endif
	}else {	
		if(imx_decoder_error_flg == TRUE){
			if(TRUE == imx_decoder_error_end){
				imx6_decoder_error_reset(&media_header, &g_imx6_codec);
			}else{
				return SOCKET_OK;
			}
		}else{
			if(TRUE == imx6_decoder_check(&media_header, &g_imx6_codec)){
				imx6_decoder_reset(&media_header, &g_imx6_codec);
				return SOCKET_OK;
			}
		}
		if(should_drop_data(&media_header) == FALSE){
			if(g_imx6_codec.fifo_fd != -1){
				memcpy(data_buf + media_header.size, (void*)&fend, sizeof(fend));
				ret = write(g_imx6_codec.fifo_fd, data_buf, TOTAL_DATA_LEN);
				if( ret != TOTAL_DATA_LEN){
					g_warning("Fifo write incompleted ret=%d, jpeg_len=%d ....................................", ret, media_header.size);
					perror("Fifo write incompleted");
					imx6_decoder_reset(&media_header, &g_imx6_codec);
				}
			}
		}
	}
	return SOCKET_OK;

}


