#include "output.h"

#include "stb_ds.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct buffer buffer_t;

struct buffer {
	short buffer[BUFSZ * 2];
};

struct output {
	FILE*	  fp;
	buffer_t* buffers;
};

static void write16(FILE* f, unsigned short n) {
	int	      i;
	unsigned char c;

	for(i = 0; i < sizeof(n); i++) {
		c = n & 0xff;
		n = n >> 8;

		fwrite(&c, 1, 1, f);
	}
}

static void write32(FILE* f, unsigned int n) {
	int	      i;
	unsigned char c;

	for(i = 0; i < sizeof(n); i++) {
		c = n & 0xff;
		n = n >> 8;

		fwrite(&c, 1, 1, f);
	}
}

static output_t* New(const char* output) {
	output_t* self = calloc(1, sizeof(*self));

	if(output == NULL || (self->fp = fopen(output, "wb")) == NULL) {
		fprintf(stderr, "cannot open output\n");

		free(self);
		return NULL;
	}

	return self;
}

static void Write(output_t* self, short* wave) {
	buffer_t buffer;

	memcpy(buffer.buffer, wave, sizeof(buffer.buffer));

	arrput(self->buffers, buffer);
}

static int BufferedSize(output_t* self) {
	return arrlen(self->buffers) * BUFSZ;
}

static void Destroy(output_t* self) {
	int samples = arrlen(self->buffers) * BUFSZ;
	int i;

	fwrite("RIFF", 1, 4, self->fp);
	write32(self->fp, 4 + (8 + 16) + (8 + samples * 2 * 2));
	fwrite("WAVE", 1, 4, self->fp);
	fwrite("fmt ", 1, 4, self->fp);
	write32(self->fp, 16);
	write16(self->fp, 1);
	write16(self->fp, 2);
	write32(self->fp, RATE);
	write32(self->fp, RATE * 2 * 2);
	write16(self->fp, 2 * 2);
	write16(self->fp, 16);
	fwrite("data", 1, 4, self->fp);
	write32(self->fp, samples * 2 * 2);

	for(i = 0; i < arrlen(self->buffers); i++) {
		int j;

		for(j = 0; j < BUFSZ * 2; j++) {
			write16(self->fp, self->buffers[i].buffer[j]);
		}
	}

	arrfree(self->buffers);
	fclose(self->fp);
	free(self);
}

static output_mod_t output = {
    New,
    Write,
    BufferedSize,
    Destroy,
    1};
output_mod_t* o_wave = &output;
