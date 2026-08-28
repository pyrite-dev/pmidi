#include <turbosynth/midi.h>
#include <turbosynth/guspat.h>

#include "miniaudio.h"
#include "stb_ds.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define BUFSZ 800

typedef struct buffer buffer_t;

struct buffer {
	short buffer[BUFSZ * 2];
	int   seek;
};

static GUSPatSynth* gGUSPatSynth;

static ma_mutex	 gBufferMutex;
static buffer_t* gBuffer = NULL;

static void callback(MidiStream* ms, const MidiEvent* event) {
	if(event->type == MidiEventNote) {
		GUSPatSynth_Note(gGUSPatSynth, event->note.channel, event->note.key, event->note.velocity);
	} else if(event->type == MidiEventControl) {
		if(event->control.key == MidiControlBankSelectMSB) {
			GUSPatSynth_SetBankMSB(gGUSPatSynth, event->control.channel, event->control.value);
		} else if(event->control.key == MidiControlBankSelectLSB) {
			GUSPatSynth_SetBankLSB(gGUSPatSynth, event->control.channel, event->control.value);
		}
	} else if(event->type == MidiEventProgramChange) {
		int drum = 0;

		if(gGUSPatSynth->channels[event->programChange.channel].bankMsb == 120 || event->programChange.channel == 9) drum = 1;

		GUSPatSynth_SetProgram(gGUSPatSynth, event->programChange.channel, event->programChange.program, drum);
	}
}

static void render(short* out, int frames) {
	GUSPatSynth_RenderShort(gGUSPatSynth, out, frames);
}

static int bufferSize(void) {
	int i;
	int r = 0;

	ma_mutex_lock(&gBufferMutex);
	for(i = 0; i < arrlen(gBuffer); i++) {
		r += BUFSZ - gBuffer[i].seek;
	}
	ma_mutex_unlock(&gBufferMutex);

	return r;
}

static void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frames) {
	short* out = output;
	int    f   = 0;

	memset(out, 0, sizeof(*out) * frames * 2);
	while(bufferSize() > 0 && (frames - f) > 0) {
		int n = 0;

		ma_mutex_lock(&gBufferMutex);
		n = (frames - f) > (BUFSZ - gBuffer[0].seek) ? (BUFSZ - gBuffer[0].seek) : (frames - f);

		memcpy(out + f * 2, gBuffer[0].buffer + gBuffer[0].seek * 2, sizeof(gBuffer[0].buffer[0]) * n * 2);

		gBuffer[0].seek += n;
		f += n;
		if((BUFSZ - gBuffer[0].seek) <= 0) arrdel(gBuffer, 0);
		ma_mutex_unlock(&gBufferMutex);
	}
}

int main(int argc, char** argv) {
	FileStream*	 fs;
	FileStream*	 cfgfs;
	MidiStream*	 ms;
	ma_device_config config;
	ma_device	 device;

	if(argc != 3) {
		fprintf(stderr, "Usage: %s cfg midi\n", argv[0]);
		return 1;
	}

	if((cfgfs = FileStream_New(argv[1], NULL)) == NULL) {
		fprintf(stderr, "cannot open cfg\n");
		return 1;
	}

	if((gGUSPatSynth = GUSPatSynth_New(cfgfs, 48000)) == NULL) {
		FileStream_Destroy(cfgfs);

		fprintf(stderr, "cannot open gus patches\n");
		return 1;
	}

	FileStream_Destroy(cfgfs);

	if((fs = FileStream_New(argv[2], NULL)) == NULL) {
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

	config			 = ma_device_config_init(ma_device_type_playback);
	config.playback.format	 = ma_format_s16;
	config.playback.channels = 2;
	config.sampleRate	 = 48000;
	config.dataCallback	 = dataCallback;
	config.pUserData	 = NULL;

	if(ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
		MidiStream_Destroy(ms);
		FileStream_Destroy(fs);
		GUSPatSynth_Destroy(gGUSPatSynth);

		fprintf(stderr, "cannot open audio\n");
		return 1;
	}

	if(ma_device_start(&device) != MA_SUCCESS) {
		ma_device_uninit(&device);
		MidiStream_Destroy(ms);
		FileStream_Destroy(fs);
		GUSPatSynth_Destroy(gGUSPatSynth);

		fprintf(stderr, "cannot open audio\n");
		return 1;
	}

	ma_mutex_init(&gBufferMutex);

	while(1) {
		buffer_t buffer = {0};
		int	 i;

		for(i = 0; i < ms->nTracks && ms->tracks[i].finished; i++);
		if(i == ms->nTracks) break;

		MidiStream_Advance(ms, (double)BUFSZ / 48000);

		render(buffer.buffer, BUFSZ);

		ma_mutex_lock(&gBufferMutex);
		arrput(gBuffer, buffer);
		ma_mutex_unlock(&gBufferMutex);

		while(bufferSize() > 4800)
#ifdef _WIN32
			Sleep(1);
#else
			usleep(1000);
#endif
	}

quit:;
	ma_mutex_uninit(&gBufferMutex);
	ma_device_uninit(&device);
	MidiStream_Destroy(ms);
	FileStream_Destroy(fs);
	GUSPatSynth_Destroy(gGUSPatSynth);

	arrfree(gBuffer);

	return 0;
}
