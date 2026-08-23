#include <midi.h>
#include <SDL.h>

// #define USE_SF2

#ifdef USE_SF2
#include "tsf.h"

tsf* gTsf;
#else
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265
#endif
#endif

#define BUFSZ 240

#ifndef USE_SF2
#define VOICES 16

typedef struct voice {
	double freq;
	double vel;
	double x;
} voice_t;

typedef struct channel {
	voice_t voices[VOICES];
} channel_t;
channel_t channels[128] = {0};
#endif

static void callback(MidiStream* ms, const MidiEvent* event) {
#ifdef USE_SF2
	if(event->type == MidiEventNote) {
		tsf_channel_note_on(gTsf, event->note.channel, event->note.key, event->note.velocity / 127.0);
	} else if(event->type == MidiEventProgramChange) {
		tsf_channel_set_presetnumber(gTsf, event->programChange.channel, event->programChange.program, event->programChange.channel == 9 ? 1 : 0);
	}
#else
	if(event->type == MidiEventNote) {
		channel_t* ch = &channels[event->note.channel];
		int	   i;
		double	   freq = 440 * pow(2, (double)(event->note.key - 69) / 12);

		if(event->note.velocity == 0) {
			for(i = 0; i < VOICES; i++) {
				if(ch->voices[i].freq == freq) {
					ch->voices[i].freq = 0;
					ch->voices[i].vel  = 0;
					ch->voices[i].x	   = 0;

					break;
				}
			}
		} else {
			for(i = 0; i < VOICES && ch->voices[i].vel != 0; i++);

			if(i < VOICES) {
				ch->voices[i].freq = freq;
				ch->voices[i].vel  = event->note.velocity / 127.0;
				ch->voices[i].x	   = 0;
			}
		}
	}
#endif
}

static void render(short* out, int frames) {
#ifdef USE_SF2
	tsf_render_short(gTsf, out, frames, 0);
#else
	int i, j, k;

	memset(out, 0, frames * 2 * 2);

	for(j = 0; j < frames; j++) {
		float n = 0;

		for(i = 0; i < 128; i++) {
			for(k = 0; k < VOICES; k++) {
				if(channels[i].voices[k].vel == 0) continue;

				n += sin(2 * M_PI * channels[i].voices[k].freq * channels[i].voices[k].x) * channels[i].voices[k].vel / (channels[i].voices[k].x * 4 + 1) / 4;

				channels[i].voices[k].x += 1.0 / 48000;
			}
		}

		if(-1 > n) n = -1;
		if(1 < n) n = 1;

		out[2 * j + 0] = out[2 * j + 1] = n * 4096;
	}
#endif
}

int main(int argc, char** argv) {
	FileStream*	  fs;
	MidiStream*	  ms;
	SDL_AudioSpec	  spec;
	SDL_AudioDeviceID dev;

	if(argc != 3) {
		fprintf(stderr, "Usage: %s sf2 midi\n", argv[0]);
		return 1;
	}

#ifdef USE_SF2
	if((gTsf = tsf_load_filename(argv[1])) == NULL) {
		fprintf(stderr, "cannot open sf2\n");
		return 1;
	}

	tsf_set_output(gTsf, TSF_STEREO_INTERLEAVED, 48000, -10);
#endif

	if((fs = FileStream_New(argv[2])) == NULL) {
#ifdef USE_SF2
		tsf_close(gTsf);
#endif

		fprintf(stderr, "cannot open midi\n");
		return 1;
	}

	if((ms = MidiStream_New(fs, callback)) == NULL) {
#ifdef USE_SF2
		tsf_close(gTsf);
#endif
		FileStream_Destroy(fs);

		fprintf(stderr, "cannot open midi\n");
		return 1;
	}

	SDL_Init(SDL_INIT_AUDIO);

	memset(&spec, 0, sizeof(spec));
	spec.freq     = 48000;
	spec.format   = AUDIO_S16;
	spec.channels = 2;
	spec.samples  = 2048;

	if((dev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0)) == 0) {
#ifdef USE_SF2
		tsf_close(gTsf);
#endif
		FileStream_Destroy(fs);
		MidiStream_Destroy(ms);

		fprintf(stderr, "cannot open audio\n");
		return 1;
	}

	SDL_PauseAudioDevice(dev, 0);

	while(1) {
		SDL_Event ev;
		short	  buffer[BUFSZ * 2] = {0};

		while(SDL_PollEvent(&ev)) {
			if(ev.type == SDL_QUIT) goto quit;
		}

		MidiStream_Advance(ms, (double)BUFSZ / 48000);

		render(buffer, BUFSZ);
		SDL_QueueAudio(dev, buffer, BUFSZ * 2 * 2);

		if(SDL_GetQueuedAudioSize(dev) > 48000) SDL_Delay((double)BUFSZ / 48000 * 1000);
	}

quit:;
#ifdef USE_SF2
	tsf_close(gTsf);
#endif
	FileStream_Destroy(fs);
	MidiStream_Destroy(ms);

	return 0;
}
