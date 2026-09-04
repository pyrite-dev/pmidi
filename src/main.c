#include <turbosynth/easymidi.h>

#include "miniaudio.h"
#include "stb_ds.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define RATE 48000
#define BUFSZ (RATE / 100)

typedef struct buffer buffer_t;

struct buffer {
	short buffer[BUFSZ * 2];
	int   seek;
};

static EasyMidi* gEasyMidi;

static ma_mutex	 gBufferMutex;
static buffer_t* gBuffer = NULL;

static void render(short* out, int frames) {
	EasyMidi_RenderShort(gEasyMidi, out, frames);
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
	ma_device_config config;
	ma_device	 device;
	const char* cfg = NULL;
	const char* midi = NULL;
	int i;

	for(i = 1; i < argc; i++){
		if(strcmp(argv[i], "-C") == 0){
			cfg = argv[++i];
		}else if(argv[i][0] == '-'){
		}else{
			midi = argv[i];
		}
	}

	if(cfg == NULL){
		fprintf(stderr, "specify config file\n");
		return 1;
	}

	if(midi == NULL){
		fprintf(stderr, "specify midi file\n");
		return 1;
	}

	if((gEasyMidi = EasyMidi_New(cfg, RATE)) == NULL) {
		fprintf(stderr, "cannot open gus patches\n");
		return 1;
	}

	if(!EasyMidi_Load(gEasyMidi, midi)) {
		EasyMidi_Destroy(gEasyMidi);

		fprintf(stderr, "cannot open midi\n");
		return 1;
	}

	config			 = ma_device_config_init(ma_device_type_playback);
	config.playback.format	 = ma_format_s16;
	config.playback.channels = 2;
	config.sampleRate	 = RATE;
	config.dataCallback	 = dataCallback;
	config.pUserData	 = NULL;

	if(ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
		EasyMidi_Destroy(gEasyMidi);

		fprintf(stderr, "cannot open audio\n");
		return 1;
	}

	if(ma_device_start(&device) != MA_SUCCESS) {
		ma_device_uninit(&device);
		EasyMidi_Destroy(gEasyMidi);

		fprintf(stderr, "cannot open audio\n");
		return 1;
	}

	ma_mutex_init(&gBufferMutex);

	while(1) {
		buffer_t buffer = {0};
		int	 i;

		if(EasyMidi_IsFinished(gEasyMidi)) break;

		render(buffer.buffer, BUFSZ);

		ma_mutex_lock(&gBufferMutex);
		arrput(gBuffer, buffer);
		ma_mutex_unlock(&gBufferMutex);

		while(bufferSize() > RATE / 10)
#ifdef _WIN32
			Sleep(1);
#else
			usleep(1000);
#endif
	}

	while(bufferSize() > 0);

quit:;
	ma_mutex_uninit(&gBufferMutex);
	ma_device_uninit(&device);
	EasyMidi_Destroy(gEasyMidi);

	arrfree(gBuffer);

	return 0;
}
