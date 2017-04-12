#include "imx-decoder.h"
#include <netdb.h>

int imx_decoder_error_flg = FALSE;
int imx_decoder_error_end = FALSE;
int imx_decoder_reset_flg = FALSE;

int wait_h264_i_frame_flg = TRUE;


int exit_decode_flag=0;

static vpudec_data_t vd;

imx6_codec_t g_imx6_codec = {
	.fifo_fd = -1,
	.type = FRAME_TYPE_H264_I,
	.tstate = none,
	.width = 1600,
	.height = 900,
};

void start_decode_process(imx6_codec_t* pcodec);
void stop_decode_thread(imx6_codec_t* pcodec);

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop *) data;
	//printf("bus_call msg = %d\n", GST_MESSAGE_TYPE (msg));
	switch (GST_MESSAGE_TYPE (msg)) {
		case GST_MESSAGE_EOS:
			g_print ("End of stream\n");
			//g_main_loop_quit (loop);
			//imx_decoder_error_flg = TRUE;
			//stop_decode_thread(&g_imx6_codec);
			break;
		case GST_MESSAGE_ERROR: {
			gchar *debug;
			GError *error;
			gst_message_parse_error (msg, &error, &debug);
			g_free (debug);
			g_printerr ("[bus_call] Error: %s\n", error->message);
			//g_printerr ("[bus_call] Error===========: \n");
			g_error_free (error);
			//g_main_loop_quit (loop);
			imx_decoder_error_flg = TRUE;
			stop_decode_thread(&g_imx6_codec);
			break;
		}
		default:
					break;
	}
	return TRUE;
}
//#if 1

static GstBusSyncReply bus_sync_handler (GstBus * bus, GstMessage * message, gpointer user_data)
{
#if 0
	if (GST_MESSAGE_TYPE (message) != GST_MESSAGE_ELEMENT)
		return GST_BUS_PASS;
	if (!gst_structure_has_name (message->structure, "prepare-xwindow-id"))
		return GST_BUS_PASS;

	if (video_window_xid != 0) {
		GstXOverlay *xoverlay;
		xoverlay = GST_X_OVERLAY (GST_MESSAGE_SRC (message));
		gst_x_overlay_set_window_handle (xoverlay, video_window_xid);
		printf("--------------gst_x_overlay_set_window_handle ---------------------\n");
	} else {
		printf("bus_sync_handler:Should have obtained video_window_xid by now!");
	}

	gst_message_unref (message);
	return GST_BUS_DROP;
#endif
}

#if 0 //for arm cpu
static void *decode_thread (void *arg)
{
	GstBus *bus;
	guint bus_watch_id;
	GstCaps *caps;
	gboolean link_ok;
	vpudec_data_t *vdp = (vpudec_data_t*)arg;

	g_message("decode_thread: entered ...\n");
	gst_init (NULL, NULL);
	vdp->loop = g_main_loop_new (NULL, FALSE);
	
	vdp->pipeline = gst_pipeline_new ("imx6-vpu-decode");
	vdp->source = gst_element_factory_make ("filesrc","file-source");
	vdp->decoder = gst_element_factory_make ("vpudec","vpu-decode");
	vdp->sink = gst_element_factory_make ("mfw_v4lsink", "fake-sink");
	if (!vdp->pipeline || !vdp->source || !vdp->decoder || !vdp->sink) {
		g_printerr ("One element could not be created. Exiting.\n");
		g_printerr ("source=%p decode=%p sink=%p\n", vdp->source, vdp->decoder, vdp->sink);
		return NULL;
	}
	g_object_set (G_OBJECT (vdp->source), "location", DATA_FIFO, NULL);
	g_object_set (G_OBJECT (vdp->source), "blocksize", TOTAL_DATA_LEN, NULL);
	g_object_set (G_OBJECT (vdp->sink), "sync", 0, NULL);
	g_object_set (G_OBJECT (vdp->decoder), "low-latency", 1, NULL);
	
	bus = gst_pipeline_get_bus (GST_PIPELINE (vdp->pipeline));
	bus_watch_id = gst_bus_add_watch (bus, bus_call, vdp->loop);
	//gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, NULL);
	gst_object_unref (bus);
	gst_bin_add_many (GST_BIN (vdp->pipeline), vdp->source, vdp->decoder, vdp->sink, NULL);
	if(vdp->type == FRAME_TYPE_MJPEG){
		caps = gst_caps_new_simple("image/jpeg", NULL);
	}else if(vdp->type == FRAME_TYPE_H264_I || vdp->type == FRAME_TYPE_H264_P){
		caps = gst_caps_new_simple("video/x-h264", NULL);
	}else{
		g_error("------------ unkown encode type-------------\n");
	}
	link_ok = gst_element_link_filtered(vdp->source, vdp->decoder, caps);
	gst_caps_unref(caps);
	if(! link_ok)
		g_warning("Failed to link filter and sink");

	gst_element_link_many (vdp->decoder, vdp->sink, NULL);
	g_print ("Now playing: \n");
	imx_decoder_error_flg = FALSE;
	imx_decoder_error_end = FALSE;
	gst_element_set_state (vdp->pipeline, GST_STATE_PLAYING);
	g_print ("Running...\n");
	g_main_loop_run (vdp->loop);

	g_print ("Returned, stopping playback\n");
	gst_element_set_state (vdp->pipeline, GST_STATE_NULL);
	g_print ("Deleting pipeline\n");
	gst_object_unref (GST_OBJECT (vdp->pipeline));
	g_source_remove (bus_watch_id);
	g_main_loop_unref (vdp->loop);
	if(TRUE == imx_decoder_error_flg){
		imx_decoder_error_end = TRUE;
	}
	imx_decoder_reset_flg = TRUE;
	return NULL;
}
#else //for x86 cpu
static void *decode_thread (void *arg)
{
	GstBus *bus;
	guint bus_watch_id;
	GstCaps *caps;
	gboolean link_ok;
	GstStateChangeReturn gstret = GST_STATE_CHANGE_FAILURE;
	vpudec_data_t *vdp = (vpudec_data_t*)arg;

	g_message("decode_thread: entered ...\n");
	gst_init (NULL, NULL);
	vdp->loop = g_main_loop_new (NULL, FALSE);

	vdp->pipeline = gst_pipeline_new ("vaapi-decode");
	if ( !vdp->pipeline) { 
		g_printerr (" pipeline could not  be created.\n"); 
		return NULL; 
	} 
	vdp->source = gst_element_factory_make ("filesrc", "file-source");
	if ( !vdp->source) { 
		g_printerr (" source could not  be created.\n"); 
		return NULL; 
	} 
	vdp->parse = gst_element_factory_make ("h264parse"/*"vaapiparse_h264"*/, "vaapiparse");
	if ( !vdp->parse) { 
		g_printerr (" parse could not  be created.\n"); 
		return NULL; 
	}
	vdp->decoder = gst_element_factory_make ("vaapidecode", "decoder");
	if ( !vdp->decoder) { 
		g_printerr (" decoder could not  be created.\n"); 
		return NULL; 
	}
	vdp->sink = gst_element_factory_make ("vaapisink", "sink");
	if ( !vdp->sink) { 
		g_printerr (" sink could not  be created.\n"); 
		return NULL; 
	}
	
	
	g_object_set (G_OBJECT (vdp->source), "location", DATA_FIFO, NULL);
	g_object_set (G_OBJECT (vdp->source), "blocksize", TOTAL_DATA_LEN, NULL);

	g_object_set (G_OBJECT (vdp->sink), "sync", 0, NULL);
        g_object_set (G_OBJECT (vdp->sink), "display", 2, NULL); //va/glx display
    	g_object_set (G_OBJECT (vdp->sink), "fullscreen", 1, NULL);
    	
	bus = gst_pipeline_get_bus (GST_PIPELINE (vdp->pipeline));
	bus_watch_id = gst_bus_add_watch (bus, bus_call, vdp->loop);
	//gst_bus_set_sync_handler (bus, (GstBusSyncHandler) bus_sync_handler, NULL);
	gst_object_unref (bus);
	gst_bin_add_many (GST_BIN (vdp->pipeline), vdp->source, vdp->parse, vdp->decoder, vdp->sink, NULL);
	#if 0
	if(vdp->type == FRAME_TYPE_MJPEG){
		caps = gst_caps_new_simple ("video/jpeg",
			"width", G_TYPE_INT, g_imx6_codec.width,
			"height", G_TYPE_INT, g_imx6_codec.height,
			"framerate", GST_TYPE_FRACTION, 25, 1,
			NULL);
	}else if(vdp->type == FRAME_TYPE_H264_I || vdp->type == FRAME_TYPE_H264_P){
		/*
		caps = gst_caps_new_simple ("video/x-h264",
			"width", G_TYPE_INT, g_imx6_codec.width,
			"height", G_TYPE_INT, g_imx6_codec.height,
			"framerate", GST_TYPE_FRACTION, 0, 1000,
			NULL);
		*/
		caps = gst_caps_new_simple ("video/x-h264",
			"alignment", G_TYPE_STRING, "au",
			"stream-format", G_TYPE_STRING, "avc",
			NULL);
	}else{
		g_error("------------ unkown encode type-------------\n");
	}
	link_ok = gst_element_link_filtered(vdp->source, vdp->decoder, caps);
	gst_caps_unref(caps);
	if(! link_ok)
		g_warning("Failed to link filter and sink");
	#endif
	gst_element_link_many(vdp->source, vdp->parse, vdp->decoder, vdp->sink, NULL);
	
	g_print ("Now playing: \n");
	imx_decoder_error_flg = FALSE;
	imx_decoder_error_end = FALSE;
	gstret = gst_element_set_state (vdp->pipeline, GST_STATE_PLAYING);
	while(gstret == GST_STATE_CHANGE_FAILURE){
		sleep(1);
		gstret = gst_element_set_state (vdp->pipeline, GST_STATE_PLAYING);
		g_print ("set pipeline state [playing] fail\n");
	}
	g_print ("Running...\n");
	g_main_loop_run (vdp->loop);

	g_print ("Returned, stopping playback\n");
	gst_element_set_state (vdp->pipeline, GST_STATE_NULL);
	g_print ("Deleting pipeline\n");
	gst_object_unref (GST_OBJECT (vdp->pipeline));
	g_source_remove (bus_watch_id);
	g_main_loop_unref (vdp->loop);
	if(TRUE == imx_decoder_error_flg){
		imx_decoder_error_end = TRUE;
	}
	imx_decoder_reset_flg = TRUE;
	g_print ("decode_thread finished\n");
	return NULL;
}
#endif 


void stop_decode_thread(imx6_codec_t* pcodec)
{
	printf("%s\n", __FUNCTION__);fflush(stdout);
	if(pcodec->fifo_fd != -1){
		close(pcodec->fifo_fd);
		pcodec->fifo_fd = -1;
	}
	pcodec->tstate = stopped;
	g_main_loop_quit (vd.loop);
}

void close_decoder()
{
	stop_decode_thread(&g_imx6_codec);
}


int imx6_decoder_reset(struct media_header* header,imx6_codec_t* pcode)
{
	printf("-------- imx6_decoder_reset ----------\n");fflush(stdout);
	printf("fifo_fd==%d\n",pcode->fifo_fd);fflush(stdout);
	if(pcode->tstate == running){
		imx_decoder_reset_flg = FALSE;
		stop_decode_thread(pcode);
		while(imx_decoder_reset_flg == FALSE){
			sleep(1); //waitting for decoder thread quit
			printf("waitting for decoder stop\n"); fflush(stdout);
		}
	}
	pcode->type = header->type;
	pcode->width = header->width;
	pcode->height = header->height;
	start_decode_process(pcode);

	wait_h264_i_frame_flg = TRUE;
	return 0;
}

void imx6_decoder_error_reset(struct media_header* header, imx6_codec_t* pcodec)
{

	printf("-------- imx6_decoder_error_reset ----------\n");
	fflush(stdout);
	pcodec->type = header->type;
	pcodec->width = header->width;
	pcodec->height = header->height;
	start_decode_process(pcodec);
	
	if(FRAME_TYPE_H264_I == header->type){
		wait_h264_i_frame_flg = FALSE;

	}else{
		wait_h264_i_frame_flg = TRUE;
	}
}




int start_imx6_decoder(imx6_codec_t* pcodec)
{
	int ret;

	vd.jpeg_w = pcodec->width;
	vd.jpeg_h = pcodec->height;
	vd.disp_x = 0;
	vd.disp_y = 0;
	vd.disp_w = pcodec->width;
	vd.disp_h = pcodec->height;
	vd.type = pcodec->type;

	g_message("decode_thread creating...");
	printf("codec type=%d width=%d height=%d\n", pcodec->type, pcodec->width, pcodec->height);
	fflush(stdout);
	ret = pthread_create(&(vd.thread_id), NULL, decode_thread, &vd);
	if( ret !=0 ){
		perror("create pthread(decode) error");
		exit(1);

	}

	pthread_detach(vd.thread_id);
	fprintf(stderr, "decode_thread created\n");

	return 0;
}


int imx6_decoder_check(struct media_header* header, imx6_codec_t* current_dec)
{
	int ret = FALSE;

	if(header->width != current_dec->width || header->height != current_dec->height){
		ret =  TRUE;
	}else if(header->type != current_dec->type){
		if(header->type == FRAME_TYPE_MJPEG || current_dec->type == FRAME_TYPE_MJPEG){
			ret = TRUE;
		}
	}
	if(ret == TRUE){
		printf("pcodec->type=%d\n", header->type);
		printf("pcodec->width=%d\n", header->width);
		printf("pcodec->height=%d\n", header->height);
		printf("current_dec->type=%d\n", current_dec->type);
		printf("current_dec->width=%d\n", current_dec->width);
		printf("current_dec->height=%d\n", current_dec->height);
	}
	return ret;
}

void start_decode_process(imx6_codec_t* pcodec)
{

	printf("start_decode_process: entered...\n");fflush(stdout);
	unlink(DATA_FIFO);
	umask(0);
	if( mkfifo(DATA_FIFO, 0666) < 0 ){
		perror("Create FIFO error");
	}
	unlink("/tmp/vpuproc");


	pcodec->tstate = starting;

	start_imx6_decoder(pcodec);
	printf("open data pipe\n");fflush(stdout);
	pcodec->fifo_fd = open(DATA_FIFO, O_WRONLY | O_DSYNC);
	if(pcodec->fifo_fd == -1 ){
		perror( "Open write " DATA_FIFO);
		return ;
	}
	printf("decode_struct.fifo_fd = %d\n", pcodec->fifo_fd);
	pcodec->tstate = running;
	printf("start_decode_process: finished...\n");
}

//--------------------------------------------------

