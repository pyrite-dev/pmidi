#include "output.h"

#include "miniaudio.h"
#include "stb_ds.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

typedef struct buffer buffer_t;

struct buffer {
	short buffer[BUFSZ * 2];
	int   seek;
};

struct output {
	ma_mutex	 mutex;
	buffer_t*	 buffers;
	ma_device_config config;
	ma_device	 device;
};

static int bufferSize(output_t* self) {
	int i;
	int r = 0;

	ma_mutex_lock(&self->mutex);
	for(i = 0; i < arrlen(self->buffers); i++) {
		r += BUFSZ - self->buffers[i].seek;
	}
	ma_mutex_unlock(&self->mutex);

	return r;
}

static void dataCallback(ma_device* device, void* output, const void* input, ma_uint32 frames) {
	short*	  out  = output;
	int	  f    = 0;
	output_t* self = device->pUserData;

	memset(out, 0, sizeof(*out) * frames * 2);
	while(bufferSize(self) > 0 && (frames - f) > 0) {
		int n = 0;

		ma_mutex_lock(&self->mutex);
		n = (frames - f) > (BUFSZ - self->buffers[0].seek) ? (BUFSZ - self->buffers[0].seek) : (frames - f);

		memcpy(out + f * 2, self->buffers[0].buffer + self->buffers[0].seek * 2, sizeof(self->buffers[0].buffer[0]) * n * 2);

		self->buffers[0].seek += n;
		f += n;
		if((BUFSZ - self->buffers[0].seek) <= 0) arrdel(self->buffers, 0);
		ma_mutex_unlock(&self->mutex);
	}
}

static output_t* New(const char* output) {
	output_t* self = calloc(1, sizeof(*self));

	ma_mutex_init(&self->mutex);

	self->config		       = ma_device_config_init(ma_device_type_playback);
	self->config.playback.format   = ma_format_s16;
	self->config.playback.channels = 2;
	self->config.sampleRate	       = RATE;
	self->config.dataCallback      = dataCallback;
	self->config.pUserData	       = self;

	if(ma_device_init(NULL, &self->config, &self->device) != MA_SUCCESS) {
		fprintf(stderr, "cannot open audio\n");

		free(self);
		return NULL;
	}

	if(ma_device_start(&self->device) != MA_SUCCESS) {
		ma_device_uninit(&self->device);
		fprintf(stderr, "cannot open audio\n");

		free(self);
		return NULL;
	}

	return self;
}

static void Write(output_t* self, short* wave) {
	buffer_t buffer;

	memcpy(buffer.buffer, wave, sizeof(buffer.buffer));
	buffer.seek = 0;

	ma_mutex_lock(&self->mutex);
	arrput(self->buffers, buffer);
	ma_mutex_unlock(&self->mutex);

	while(bufferSize(self) >= RATE * 0.5);
#ifdef _WIN32
	Sleep(1);
#else
	usleep(1 * 1000);
#endif
}

static void Destroy(output_t* self) {
	ma_device_uninit(&self->device);
	arrfree(self->buffers);
	ma_mutex_uninit(&self->mutex);
	free(self);
}

static output_mod_t output = {
    New,
    Write,
    Destroy};
output_mod_t* o_miniaudio = &output;
