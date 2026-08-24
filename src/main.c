#include <midi.h>
#include <guspat.h>
#include <SDL.h>

#include <math.h>

static GUSPatSynth* gGUSPatSynth;
static int	    gUseGUSPatSynth;

#ifndef M_PI
#define M_PI 3.14159265
#endif

#define BUFSZ 240

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

static void callback(MidiStream* ms, const MidiEvent* event) {
	if(gUseGUSPatSynth) {
		if(event->type == MidiEventNote) {
			GUSPatSynth_Note(gGUSPatSynth, event->note.channel, event->note.key, event->note.velocity);
		} else if(event->type == MidiEventProgramChange) {
			GUSPatSynth_SetProgram(gGUSPatSynth, event->programChange.channel, event->programChange.program, event->programChange.channel == 9 ? 1 : 0);
		}
	} else {
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
	}
}

static void render(short* out, int frames) {
	if(gUseGUSPatSynth) {
		GUSPatSynth_RenderShort(gGUSPatSynth, out, frames);
	} else {
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
	}
}

int main(int argc, char** argv) {
	FileStream*	  fs;
	MidiStream*	  ms;
	SDL_AudioSpec	  spec;
	SDL_AudioDeviceID dev;

	if(argc != 3 && argc != 2) {
		fprintf(stderr, "Usage: %s [cfg] midi\n", argv[0]);
		return 1;
	}

	gUseGUSPatSynth = argc == 3 ? 1 : 0;

	if(gUseGUSPatSynth) {
		FileStream* cfgfs;

		if((cfgfs = FileStream_New(argv[1])) == NULL) {
			fprintf(stderr, "cannot open cfg\n");
			return 1;
		}

		if((gGUSPatSynth = GUSPatSynth_New(cfgfs, 48000)) == NULL) {
			FileStream_Destroy(cfgfs);

			fprintf(stderr, "cannot open gus patches\n");
			return 1;
		}

		FileStream_Destroy(cfgfs);
	}

	if((fs = FileStream_New(gUseGUSPatSynth ? argv[2] : argv[1])) == NULL) {
		if(gUseGUSPatSynth) GUSPatSynth_Destroy(gGUSPatSynth);

		fprintf(stderr, "cannot open midi\n");
		return 1;
	}

	if((ms = MidiStream_New(fs, callback)) == NULL) {
		if(gUseGUSPatSynth) GUSPatSynth_Destroy(gGUSPatSynth);
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
		if(gUseGUSPatSynth) GUSPatSynth_Destroy(gGUSPatSynth);
		FileStream_Destroy(fs);
		MidiStream_Destroy(ms);

		fprintf(stderr, "cannot open audio\n");
		return 1;
	}

	SDL_PauseAudioDevice(dev, 0);

	while(1) {
		SDL_Event ev;
		short	  buffer[BUFSZ * 2] = {0};
		int	  i;

		while(SDL_PollEvent(&ev)) {
			if(ev.type == SDL_QUIT) goto quit;
		}

		for(i = 0; i < ms->nTracks && ms->tracks[i].finished; i++);
		if(i == ms->nTracks) break;

		MidiStream_Advance(ms, (double)BUFSZ / 48000);

		render(buffer, BUFSZ);
		SDL_QueueAudio(dev, buffer, BUFSZ * 2 * 2);

		if(SDL_GetQueuedAudioSize(dev) > 48000) SDL_Delay((double)BUFSZ / 48000 * 1000);
	}

quit:;
	if(gUseGUSPatSynth) GUSPatSynth_Destroy(gGUSPatSynth);
	FileStream_Destroy(fs);
	MidiStream_Destroy(ms);

	return 0;
}
