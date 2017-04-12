#include "audio_pulse.h"
#if USE_ALSA_LIB 
/*
This example reads from the default PCM device 
and writes to standard output for 5 seconds of data. 
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>  

#include <alsa/asoundlib.h> 

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API  
#define REING_SIZE_MS 300
#define false	0
#define true	1   
snd_pcm_t *g_audio_handle;
const int frame_size = 256; 
  
int audio_playback_init()
{
 #if 0
 	int ret = -1;
 	int rc; 
 	int rate;
	int size;  

	snd_pcm_hw_params_t *params;  
	int dir;  
	snd_pcm_uframes_t frames;  
	char *buffer;                    /* TODO */  

	/* Open PCM device for playbacking. */  
	rc = snd_pcm_open(&g_audio_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);  
	if (rc < 0) {  
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));  
		return -1;  
	}  

	/* Allocate a hardware parameters object. */  
	snd_pcm_hw_params_alloca(&params);  

	/* Fill it in with default values. */  
	snd_pcm_hw_params_any(g_audio_handle, params);  

	/* Set the desired hardware parameters. */  

	/* Interleaved mode */  
	snd_pcm_hw_params_set_access(g_audio_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);  

	/* Signed 16-bit little-endian format */  
	snd_pcm_hw_params_set_format(g_audio_handle, params, SND_PCM_FORMAT_S16_LE);  

	/* Two channels (stereo) */  
	snd_pcm_hw_params_set_channels(g_audio_handle, params, 2);  

	/* 44100 bits/second sampling rate (CD quality) */ 
	rate = 44100;
	snd_pcm_hw_params_set_rate_near(g_audio_handle, params, &rate, &dir);  

	/* Set period size to 32 frames. */  
	frames = 256;  
	snd_pcm_hw_params_set_period_size_near(g_audio_handle, params, &frames, &dir);  

	/* Write the parameters to the driver */  
	rc = snd_pcm_hw_params(g_audio_handle, params);  
	if (rc < 0) {  
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));  
		return -1; 
	}  

	/* Use a buffer large enough to hold one period */  
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);
#else
	snd_pcm_hw_params_t* _hw_params;
 	snd_pcm_sw_params_t* _sw_params;
	const char* pcm_device = "default";
	snd_pcm_format_t format;
	int err;
	unsigned int sampels_per_sec = 44100;
	unsigned int channels = 2;
	
        format = SND_PCM_FORMAT_S16_LE;
	unsigned int _sampels_per_ms = sampels_per_sec/1000;

	if ((err = snd_pcm_open(&g_audio_handle, pcm_device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
	    fprintf(stderr, "cannot open audio playback device %s %s", pcm_device, snd_strerror(err));
	    return false;
	}

    if ((err = snd_pcm_hw_params_malloc(&_hw_params)) < 0) {
        fprintf(stderr, "cannot allocate hardware parameter structure %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_sw_params_malloc(&_sw_params)) < 0) {
        fprintf(stderr, "cannot allocate software parameter structure %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_any(g_audio_handle, _hw_params)) < 0) {
        fprintf(stderr, "cannot initialize hardware parameter structure %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_rate_resample(g_audio_handle, _hw_params, 1)) < 0) {
        fprintf(stderr, "cannot set rate resample %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_access(g_audio_handle, _hw_params,
                                            SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        fprintf(stderr, "cannot set access type %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_rate(g_audio_handle, _hw_params, sampels_per_sec, 0)) < 0) {
        fprintf(stderr, "cannot set sample rate %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_channels(g_audio_handle, _hw_params, channels)) < 0) {
        fprintf(stderr, "cannot set channel count %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params_set_format(g_audio_handle, _hw_params, format)) < 0) {
        fprintf(stderr, "cannot set sample format %s", snd_strerror(err));
        return false;
    }

    snd_pcm_uframes_t buffer_size;
    buffer_size = (sampels_per_sec * REING_SIZE_MS / 1000) / frame_size * frame_size;

    if ((err = snd_pcm_hw_params_set_buffer_size_near(g_audio_handle, _hw_params, &buffer_size)) < 0) {
        fprintf(stderr, "cannot set buffer size %s", snd_strerror(err));
        return false;
    }

    int direction = 1;
    snd_pcm_uframes_t period_size = (sampels_per_sec * 20 / 1000) / frame_size * frame_size;
    if ((err = snd_pcm_hw_params_set_period_size_near(g_audio_handle, _hw_params, &period_size,
                                                      &direction)) < 0) {
        fprintf(stderr, "cannot set period size %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_hw_params(g_audio_handle, _hw_params)) < 0) {
        fprintf(stderr, "cannot set parameters %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_sw_params_current(g_audio_handle, _sw_params)) < 0) {
        fprintf(stderr, "cannot obtain sw parameters %s", snd_strerror(err));
        return false;
    }

    err = snd_pcm_hw_params_get_buffer_size(_hw_params, &buffer_size);
    if (err < 0) {
        fprintf(stderr, "unable to get buffer size for playback: %s", snd_strerror(err));
        return false;
    }

    direction = 0;
    err = snd_pcm_hw_params_get_period_size(_hw_params, &period_size, &direction);
    if (err < 0) {
        fprintf(stderr, "unable to get period size for playback: %s", snd_strerror(err));
        return false;
    }

    err = snd_pcm_sw_params_set_start_threshold(g_audio_handle, _sw_params,
                                                (buffer_size / period_size) * period_size);
    if (err < 0) {
        fprintf(stderr, "unable to set start threshold mode for playback: %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_sw_params(g_audio_handle, _sw_params)) < 0) {
        fprintf(stderr, "cannot set software parameters %s", snd_strerror(err));
        return false;
    }

    if ((err = snd_pcm_prepare(g_audio_handle)) < 0) {
        fprintf(stderr, "cannot prepare pcm device %s", snd_strerror(err));
        return false;
    }

    return true;

#endif
}

 int audio_playback_data(char* data, unsigned int size)
 {
 	int rc = -1;
 	//int i;
	//fprintf(stderr, "audio_playback_data size=%d\n", size); 

	
        rc = snd_pcm_writei(g_audio_handle, data/*+128*i*/, frame_size);  
	if (rc == -EPIPE) {  
		/* EPIPE means overrun */  
		fprintf(stderr, "overrun occurred\n");  
		snd_pcm_prepare(g_audio_handle);
		return false;
	} else if (rc < 0) {  
		//fprintf(stderr, "error from write: %s\n", snd_strerror(rc)); 
		//if (snd_pcm_recover(g_audio_handle, rc, 1) == 0) {
            	//	snd_pcm_writei(g_audio_handle, data, frame_size);
        	//}
	} else if (rc != frame_size) {  
		fprintf(stderr, "short write, write %d frames\n", rc);  
	}

	return rc;
	#if 0
	snd_pcm_sframes_t ret = snd_pcm_writei(_pcm, frame, WavePlaybackAbstract::FRAME_SIZE);
    if (ret < 0) {
        if (ret == -EAGAIN) {
            return false;
        }
        DBG(0, "err %s", snd_strerror(-ret));
        if (snd_pcm_recover(_pcm, ret, 1) == 0) {
            snd_pcm_writei(_pcm, frame, WavePlaybackAbstract::FRAME_SIZE);
        }
    }
    return true;
	#endif
 }

 int audio_playback_close()
 {
 	snd_pcm_drain(g_audio_handle);  
	snd_pcm_close(g_audio_handle); 
 }
 #if 0
/**************************************************************/  
int playback_file(unsigned rate, uint16_t channels, int fd, uint32_t total_count)  
{  
    int rc;  
    int size;  
    uint32_t left_size;  
      
    snd_pcm_hw_params_t *params;  
    int dir;  
    snd_pcm_uframes_t frames;  
    char *buffer;                    /* TODO */  
  
    /* Open PCM device for playbacking. */  
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);  
    if (rc < 0) {  
        fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));  
        exit(1);  
    }  
  
    /* Allocate a hardware parameters object. */  
    snd_pcm_hw_params_alloca(&params);  
  
    /* Fill it in with default values. */  
    snd_pcm_hw_params_any(handle, params);  
  
    /* Set the desired hardware parameters. */  
  
    /* Interleaved mode */  
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);  
  
    /* Signed 16-bit little-endian format */  
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);  
  
    /* Two channels (stereo) */  
    snd_pcm_hw_params_set_channels(handle, params, channels);  
  
    /* 44100 bits/second sampling rate (CD quality) */  
    snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);  
  
    /* Set period size to 32 frames. */  
    frames = 32;  
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);  
  
    /* Write the parameters to the driver */  
    rc = snd_pcm_hw_params(handle, params);  
    if (rc < 0) {  
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));  
        exit(1);  
    }  
  
    /* Use a buffer large enough to hold one period */  
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);  
      
    size = frames * 2 * channels; /* 2 bytes/sample(16bit), 2 channels */  
    buffer = (char *) malloc(size);  
      
 //   while (left_size > 0) {  
    while (1) {  
        rc = read(fd, buffer, size);  
        totle_size += rc;                        /* totle data size */  
        left_size = total_count - totle_size;  
        if (rc != size)  
         fprintf(stderr, "short read: read %d bytes\n", rc);  
          
        rc = snd_pcm_writei(handle, buffer, frames);  
        if (rc == -EPIPE) {  
         /* EPIPE means overrun */  
         fprintf(stderr, "overrun occurred\n");  
         snd_pcm_prepare(handle);  
        } else if (rc < 0) {  
         fprintf(stderr, "error from write: %s\n", snd_strerror(rc));  
        } else if (rc != (int)frames) {  
         fprintf(stderr, "short write, write %d frames\n", rc);  
        }  
  
        if(left_size <= 0){  
            lseek(fd, sizeof(hdr), SEEK_SET);  
            totle_size = 0;  
            printf("again \n");  
//          read(fd, &hdr, sizeof(hdr));                  
        }  
    }  
  
    snd_pcm_drain(handle);  
    snd_pcm_close(handle);  
    free(buffer);  
  
    return 0;  
}  
  
/**************************************************************/  
int play_wav(const char *fn)  
{  
    unsigned rate, channels;  
    int fd;  
  
    fd = open(fn, O_RDONLY, 0777);  
    if (fd < 0) {  
        fprintf(stderr, "playback: cannot open '%s'\n", fn);  
        return -1;  
    }  
  
    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {  
        fprintf(stderr, "playback: cannot read header\n");  
        return -1;  
    }  
    fprintf(stderr,"playback: %d ch, %d hz, %d bit, %s, file_size %ld\n",  
            hdr.num_channels, hdr.sample_rate, hdr.bits_per_sample,  
            hdr.audio_format == FORMAT_PCM ? "PCM" : "unknown", hdr.data_sz);  

	return playback_file(44100, 2, fd, hdr.data_sz);
    //return playback_file(hdr.sample_rate, hdr.num_channels, fd, hdr.data_sz);  
}  
  
int main(int argc, char **argv)  
{  
    if (argc != 2) {  
        fprintf(stderr,"usage: playback <file>\n");  
        return -1;  
    }  
  
    return play_wav(argv[1]);  
}
#endif

#else
//<span style="font-size:14px;">pacat-simple.c</span>
/***
This file is part of PulseAudio.
PulseAudio is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation; either version 2.1 of the License,
or (at your option) any later version.
PulseAudio is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.
You should have received a copy of the GNU Lesser General Public License
along with PulseAudio; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA.
***/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>

#define BUFSIZE 1024
//#define TRUE	1
//#define FALSE	0
static pa_simple *g_pa_simple = NULL;
/* The Sample format to use */
static const pa_sample_spec ss = {
	.format = PA_SAMPLE_S16LE,
	.rate = 44100,
	.channels = 2
};


int pa_playbak_init(char* app_name)
{
	int error;
	printf("%s\n", __FUNCTION__);	
	/* Create a new playback stream */
	g_pa_simple = pa_simple_new(NULL, app_name, PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error);
	printf("g_pa_simple=%p\n", g_pa_simple);
	if (!g_pa_simple) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		return FALSE;
	}
	return 	TRUE;

}

void pa_playbak_reinit(void)
{
	int error;
	printf("%s\n", __FUNCTION__);
	if(!g_pa_simple){
		return;
	}
	/* Make sure that every single sample was played */
	if (pa_simple_drain(g_pa_simple, &error) < 0) {
		fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
	}

	if (g_pa_simple){
		pa_simple_free(g_pa_simple);
	}
}

int pa_playbak_playing(uint8_t* data, uint32_t size)
{
	int error;
	/* ... and play it */
	//printf("%s size=%d\n", __FUNCTION__, size);
	if(!g_pa_simple){
		printf("%s g_pa_simple=null\n", __FUNCTION__);
		return FALSE;
	}
	if (pa_simple_write(g_pa_simple, data, (size_t)size, &error) < 0) {
		fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
		return FALSE;
	}
	return TRUE;
}

#if 0
int main(int argc, char *argv[]) {
    pa_mainloop* m = NULL;
    int ret = 1;
    char *bn, *server = NULL;

    /* Set up a new main loop */
    if (!(m = pa_mainloop_new())) {
        pa_log(_("pa_mainloop_new() failed."));
        goto quit;
    }

    mainloop_api = pa_mainloop_get_api(m);

    pa_assert_se(pa_signal_init(mainloop_api) == 0);
    pa_signal_new(SIGINT, exit_signal_callback, NULL);
    pa_signal_new(SIGTERM, exit_signal_callback, NULL);
#ifdef SIGUSR1
    pa_signal_new(SIGUSR1, sigusr1_signal_callback, NULL);
#endif
    pa_disable_sigpipe();

    if (raw) {
        if (!(stdio_event = mainloop_api->io_new(mainloop_api,
                                                 mode == PLAYBACK ? STDIN_FILENO : STDOUT_FILENO,
                                                 mode == PLAYBACK ? PA_IO_EVENT_INPUT : PA_IO_EVENT_OUTPUT,
                                                 mode == PLAYBACK ? stdin_callback : stdout_callback, NULL))) {
            pa_log(_("io_new() failed."));
            goto quit;
        }
    }

    /* Create a new connection context */
    if (!(context = pa_context_new_with_proplist(mainloop_api, NULL, proplist))) {
        pa_log(_("pa_context_new() failed."));
        goto quit;
    }

    pa_context_set_state_callback(context, context_state_callback, NULL);

    /* Connect the context */
    if (pa_context_connect(context, server, 0, NULL) < 0) {
        pa_log(_("pa_context_connect() failed: %s"), pa_strerror(pa_context_errno(context)));
        goto quit;
    }

    if (verbose) {
        if (!(time_event = pa_context_rttime_new(context, pa_rtclock_now() + TIME_EVENT_USEC, time_event_callback, NULL))) {
            pa_log(_("pa_context_rttime_new() failed."));
            goto quit;
        }
    }

    /* Run the main loop */
    if (pa_mainloop_run(m, &ret) < 0) {
        pa_log(_("pa_mainloop_run() failed."));
        goto quit;
    }

quit:
    if (stream)
        pa_stream_unref(stream);

    if (context)
        pa_context_unref(context);

    if (stdio_event) {
        pa_assert(mainloop_api);
        mainloop_api->io_free(stdio_event);
    }

    if (time_event) {
        pa_assert(mainloop_api);
        mainloop_api->time_free(time_event);
    }

    if (m) {
        pa_signal_done();
        pa_mainloop_free(m);
    }

    pa_xfree(buffer);

    pa_xfree(server);
    pa_xfree(device);

    if (sndfile)
        sf_close(sndfile);

    if (proplist)
        pa_proplist_free(proplist);

    return ret;
}

int main(int argc, char*argv[])
{
	int ret = 1;
	int error;
	/* replace STDIN with the specified file if needed */
	if (argc > 1) {
		int fd;
		if ((fd = open(argv[1], O_RDONLY)) < 0) {
			fprintf(stderr, __FILE__": open() failed: %s\n", strerror(errno));
			goto finish;
		}
		if (dup2(fd, STDIN_FILENO) < 0) {
			fprintf(stderr, __FILE__": dup2() failed: %s\n", strerror(errno));
			goto finish;
		}
		close(fd);
	}
	/* Create a new playback stream */
	if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
		fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
		goto finish;
	}
	for (;;) {
		uint8_t buf[BUFSIZE];
		ssize_t r;
		#if 0
		pa_usec_t latency;
		if ((latency = pa_simple_get_latency(s, &error)) == (pa_usec_t) -1) {
			fprintf(stderr, __FILE__": pa_simple_get_latency() failed: %s\n", pa_strerror(error));
			goto finish;
		}
		fprintf(stderr, "%0.0f usec \r", (float)latency);
		#endif
		/* Read some data ... */
		if ((r = read(STDIN_FILENO, buf, sizeof(buf))) <= 0) {
			if (r == 0) /* EOF */
				break;
			fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
			goto finish;
		}
		
		/* ... and play it */
		if (pa_simple_write(s, buf, (size_t) r, &error) < 0) {
			fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
			goto finish;
		}
	}
	/* Make sure that every single sample was played */
	if (pa_simple_drain(s, &error) < 0) {
		fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
		goto finish;
	}
	ret = 0;
finish:
	if (s)
		pa_simple_free(s);
	return ret;
}
#endif

#endif

