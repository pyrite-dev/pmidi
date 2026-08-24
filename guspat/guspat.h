#ifndef __GUSPAT_H__
#define __GUSPAT_H__

#include <filestream.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct GUSSample  GUSSample;
typedef struct GUSProgram GUSProgram;
typedef struct GUSVoice	  GUSVoice;
typedef struct GUSChannel GUSChannel;
typedef struct GUSPat	  GUSPat;

#define GUSPAT_VOICES 32
#define GUSPAT_CHANNELS 128

struct GUSSample {
	unsigned int lowFrequency;
	unsigned int highFrequency;
	unsigned int rootFrequency;

	unsigned int startLoop;
	unsigned int endLoop;

	float*	     wave;	  /* stereo interleaved @ self->rate Hz */
	unsigned int nWaveFrames; /* stereo frames */

	int loop;	  /* loop or not */
	int loopBi;	  /* 0 = uni, 1 = bi */
	int loopBackward; /* 0 = forward, 1 = backward */
};

struct GUSProgram {
	int masterVolume;

	GUSSample* samples;
	int	   nSamples;

	int used;
};

struct GUSVoice {
	int key;
	int velocity;

	GUSSample* sample;

	double x;
	double ratio;

	int used;
};

struct GUSChannel {
	int program; /* OR 0x80 to make it drum */

	GUSVoice voices[GUSPAT_VOICES];
};

struct GUSPat {
	int rate;

	GUSProgram programs[128 * 2]; /* OR 0x80 to make it drum program */
	GUSChannel channels[GUSPAT_CHANNELS];
};

GUSPat* GUSPat_New(FileStream* fs, int rate);
int	GUSPat_Load(GUSPat* self, int program, int drum, FileStream* fs); /* true if success */
void	GUSPat_Unload(GUSPat* self, int program, int drum);
void	GUSPat_Note(GUSPat* self, int channel, int key, int velocity);
void	GUSPat_SetProgram(GUSPat* self, int channel, int program, int drum);
void	GUSPat_RenderShort(GUSPat* self, short* output, int frames);
void	GUSPat_RenderFloat(GUSPat* self, float* output, int frames);
void	GUSPat_Destroy(GUSPat* self);

#endif
