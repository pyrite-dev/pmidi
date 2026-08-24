#include <pmidisynth/midi.h>
#include <pmidisynth/guspat.h>

#include <SDL.h>
#include <SDL_opengl.h>

#define BUFSZ 800
#define BUFFERS 40
#define VOICES GUSPATSYNTH_VOICES

typedef struct voice   voice_t;
typedef struct channel channel_t;
typedef struct piano   piano_t;

struct voice {
	int key;

	double start;
	double end;

	int used;
};

struct channel {
	voice_t voices[VOICES * 2];
};

struct piano {
	double rgb[3];

	int playing;
};

static GUSPatSynth* gGUSPatSynth;
static int	    gWinWidth, gWinHeight;
static double	    gBegin;
static piano_t	    gPiano[96];
static channel_t    gChannels[128] = {0};

/* taken from stackoverflow */
static void hsv2rgb(double* in, double* out) {
	double hh, p, q, t, ff;
	long   i;

	if(in[1] <= 0) {
		out[0] = out[1] = out[2] = in[2];
	}

	hh = in[0];
	if(hh >= 360) hh = 0;
	hh /= 60;
	i  = (long)hh;
	ff = hh - i;
	p  = in[2] * (1 - in[1]);
	q  = in[2] * (1 - (in[1] * ff));
	t  = in[2] * (1 - (in[1] * (1 - ff)));

	switch(i) {
	case 0:
		out[0] = in[2];
		out[1] = t;
		out[2] = p;
		break;
	case 1:
		out[0] = q;
		out[1] = in[2];
		out[2] = p;
		break;
	case 2:
		out[0] = p;
		out[1] = in[2];
		out[2] = t;
		break;

	case 3:
		out[0] = p;
		out[1] = q;
		out[2] = in[2];
		break;
	case 4:
		out[0] = t;
		out[1] = p;
		out[2] = in[2];
		break;
	case 5:
	default:
		out[0] = in[2];
		out[1] = p;
		out[2] = q;
		break;
	}
}

static void callback(MidiStream* ms, const MidiEvent* event) {
	if(event->type == MidiEventNote) {
		int	   i;
		channel_t* c = &gChannels[event->note.channel];

#if 0
		if(event->note.velocity == 0) {
			for(i = 0; i < VOICES; i++) {
				voice_t* v = &c->voices[i];

				if(!v->used) continue;
				if(v->end >= 0) continue;

				if(v->key == event->note.key) {
					v->end = ms->currentSec;
				}
			}
		} else if(0 <= event->note.key && event->note.key < 96) {
			voice_t v;

			v.key	= event->note.key;
			v.start = ms->currentSec;
			v.end	= -1;
			v.used = 1;

			for(i = 0; i < VOICES && c->voices[i].used; i++);

			if(i < VOICES){
				c->voices[i] = v;
			}
		}
#endif

		GUSPatSynth_Note(gGUSPatSynth, event->note.channel, event->note.key, event->note.velocity);
	} else if(event->type == MidiEventProgramChange) {
		GUSPatSynth_SetProgram(gGUSPatSynth, event->programChange.channel, event->programChange.program, event->programChange.channel == 9 ? 1 : 0);
	}
}

static void render(short* out, int frames) {
	GUSPatSynth_RenderShort(gGUSPatSynth, out, frames);
}

#define WhiteKeys 56
#define WhiteHeight 50
#define BlackHeight 30

static const int whiteBefore[12] = {
    0, 1, 1, 2, 2, 3,
    4, 4, 5, 5, 6, 6};

static int isBlack(int key) {
	int n = key % (5 + 7);

	switch(n) {
	case 1:
	case 3:
	case 6:
	case 8:
	case 10:
		return 1;
	}

	return 0;
}

static double getWidth(int key) {
	double w = (double)gWinWidth / WhiteKeys;

	if(isBlack(key)) return w / 1.25;

	return w;
}

static double getX(int key) {
	int    n = (key / 12) * 7 + whiteBefore[key % 12];
	double w = getWidth(key);

	if(isBlack(key)) return n * getWidth(0) - w / 2;

	return n * w;
}

static void drawPiano(void) {
	int i, j;

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 96; j++) {
			double x = getX(j);
			double y = (isBlack(j) ? BlackHeight : WhiteHeight) + gWinHeight - WhiteHeight;
			double w = getWidth(j);
			int    k;

			if(i == 0 && !isBlack(j)) {
				glColor3f(1, 1, 1);
			} else if(i == 1 && !isBlack(j)) {
				glColor3f(0, 0, 0);
			} else if(i == 2 && isBlack(j)) {
				glColor3f(0, 0, 0);
			} else if(i == 3 && isBlack(j)) {
				glColor3f(0, 0, 0);
			} else {
				continue;
			}

			if((i == 0 || i == 2) && gPiano[j].playing) glColor3f(gPiano[j].rgb[0], gPiano[j].rgb[1], gPiano[j].rgb[2]);

			glBegin((i == 1 || i == 3) ? GL_LINE_LOOP : GL_QUADS);
			glVertex2f(x, gWinHeight - WhiteHeight);
			glVertex2f(x + w, gWinHeight - WhiteHeight);
			glVertex2f(x + w, y);
			glVertex2f(x, y);
			glEnd();
		}
	}
}

static void drawNotes(void) {
	int    i, j, k;
	double t = (SDL_GetTicks() / 1000.0) - gBegin;
	double hsv[3];

	hsv[1] = 0.8;
	hsv[2] = 1.0;

	for(k = 0; k < 2; k++) {
		for(i = 0; i < 128; i++) {
			channel_t* c = &gChannels[i];

			for(j = 0; j < VOICES; j++) {
				voice_t* v  = &c->voices[j];
				double	 x1 = 0, x2 = 0, y1 = 0, y2 = 0;
				double	 factor = (double)BUFSZ * BUFFERS / 48000 / 4;
				double	 rgb[3];

				if(!v->used) continue;

				memcpy(rgb, gPiano[v->key].rgb, sizeof(rgb));

				x1 = getX(v->key);
				x2 = x1 + getWidth(v->key);
				y1 = (v->end < 0) ? -100 : (gWinHeight - (v->end - t) / factor * gWinHeight);
				y2 = gWinHeight - (v->start - t) / factor * gWinHeight;

				if(k == 1) glColor3f(0, 0, 0);

				glBegin(k == 0 ? GL_QUADS : GL_LINE_LOOP);
				if(k == 0) glColor3f(rgb[0], rgb[1], rgb[2]);
				glVertex2f(x1, y1);
				if(k == 0) glColor3f(rgb[0] - 0.1, rgb[1] - 0.1, rgb[2] - 0.1);
				glVertex2f(x2, y1);
				if(k == 0) glColor3f(rgb[0] - 0.1, rgb[1] - 0.1, rgb[2] - 0.1);
				glVertex2f(x2, y2);
				if(k == 0) glColor3f(rgb[0], rgb[1], rgb[2]);
				glVertex2f(x1, y2);
				glEnd();

				if(gPiano[v->key].playing == 0 && y2 >= (gWinHeight - WhiteHeight)) {
					gPiano[v->key].playing = 1;
				}

				if(y1 >= gWinHeight) {
					gPiano[v->key].playing = 0;
					c->voices[j].used      = 0;
				}
			}
		}
	}
}

int main(int argc, char** argv) {
	FileStream*	  fs;
	FileStream*	  cfgfs;
	MidiStream*	  ms;
	SDL_Window*	  window;
	SDL_Renderer*	  renderer;
	SDL_AudioSpec	  spec;
	SDL_AudioDeviceID audio;
	int		  st = 0;
	long		  delta;
	int		  i;

	if(argc != 3) {
		fprintf(stderr, "Usage: %s cfg midi\n", argv[0]);
		return 1;
	}

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

	if((fs = FileStream_New(argv[2])) == NULL) {
		GUSPatSynth_Destroy(gGUSPatSynth);

		fprintf(stderr, "cannot open midi\n");
		return 1;
	}

	if((ms = MidiStream_New(fs, callback)) == NULL) {
		FileStream_Destroy(fs);
		GUSPatSynth_Destroy(gGUSPatSynth);

		fprintf(stderr, "cannot open midi\n");
		return 1;
	}

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	if((window = SDL_CreateWindow("PMidiGL", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL)) == NULL) {
		fprintf(stderr, "cannot create window\n");

		st = 1;
		goto cleanmidi;
	}

	if((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED)) == NULL) {
		SDL_DestroyWindow(window);

		fprintf(stderr, "cannot create renderer\n");

		st = 1;
		goto cleanmidi;
	}

	memset(&spec, 0, sizeof(spec));
	spec.freq     = 48000;
	spec.format   = AUDIO_S16SYS;
	spec.channels = 2;
	spec.samples  = 1024;
	spec.callback = NULL;

	if((audio = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0)) == 0) {
		fprintf(stderr, "cannot open audio\n");

		st = 1;
		goto cleangl;
	}

	SDL_PauseAudioDevice(audio, 0);

	SDL_GetWindowSize(window, &gWinWidth, &gWinHeight);

	SDL_GL_MakeCurrent(window, renderer);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, gWinWidth, gWinHeight, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	for(i = 0; i < 96; i++) {
		double hsv[3];

		hsv[0] = i / 96.0 * 360;
		hsv[1] = 0.8;
		hsv[2] = 1;

		hsv2rgb(hsv, gPiano[i].rgb);
	}

	delta  = SDL_GetTicks();
	gBegin = (double)delta / 1000;

	while(1) {
		short	  buffer[BUFSZ * 2] = {0};
		SDL_Event ev;

		while(SDL_PollEvent(&ev)) {
			if(ev.type == SDL_QUIT) goto quit;
		}

		if((SDL_GetTicks() - delta) >= 1000 / 60) {
			delta = SDL_GetTicks();

			glClearColor(0.25, 0.25, 0.25, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			drawNotes();
			drawPiano();

			SDL_GL_SwapWindow(window);
		}

		for(i = 0; i < ms->nTracks && ms->tracks[i].finished; i++);
		if(i == ms->nTracks) break;

		while(SDL_GetQueuedAudioSize(audio) < BUFSZ * BUFFERS) {
			MidiStream_Advance(ms, (double)BUFSZ / 48000);

			render(buffer, BUFSZ);
			SDL_QueueAudio(audio, buffer, sizeof(buffer));
		}
	}

quit:;
	SDL_CloseAudioDevice(audio);
cleangl:;
	SDL_GL_MakeCurrent(window, NULL);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
cleanmidi:;
	MidiStream_Destroy(ms);
	FileStream_Destroy(fs);
	GUSPatSynth_Destroy(gGUSPatSynth);

	return st;
}
