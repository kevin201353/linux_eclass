#ifndef __AUDIO_PULSE_HEADER__
#define __AUDIO_PULSE_HEADER__

#include <pulse/glib-mainloop.h>
#include <pulse/pulseaudio.h>

#define USE_ALSA_LIB 0

//声音数据包结构
#define SOUND_FRAME_SIZE 256
struct sound_data
{
        uint32_t data[SOUND_FRAME_SIZE];
}__attribute__((packed));

struct stream {
    pa_sample_spec          spec;
    pa_stream               *stream;
    int                     state;
    pa_operation            *uncork_op;
    pa_operation            *cork_op;
    gboolean                started;
    guint                   num_underflow;
};

struct _SPulsePrivate {
    pa_glib_mainloop        *mainloop;
    pa_context              *context;
    int                     state;
    struct stream           playback;
    struct stream           record;
    guint                   last_delay;
    guint                   target_delay;
};

typedef struct _PulsePrivate PulsePrivate;

#if USE_ALSA_LIB
int audio_playback_init(void);
int audio_playback_data(char* data, unsigned int size);
int audio_playback_close(void);
#else

int pa_playbak_init(char* app_name);

void pa_playbak_reinit(void);

int pa_playbak_playing(uint8_t* data, uint32_t size);
#endif
#endif

