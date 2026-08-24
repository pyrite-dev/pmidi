#ifndef __GUSPAT_H__
#define __GUSPAT_H__

#include <filestream.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct GUSSample   GUSSample;
typedef struct GUSProgram  GUSProgram;
typedef struct GUSVoice	   GUSVoice;
typedef struct GUSChannel  GUSChannel;
typedef struct GUSPatSynth GUSPatSynth;

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

struct GUSPatSynth {
	int rate;

	GUSProgram programs[128 * 2]; /* OR 0x80 to make it drum program */
	GUSChannel channels[GUSPAT_CHANNELS];
};

GUSPatSynth* GUSPatSynth_New(FileStream* fs, int rate);
int	     GUSPatSynth_Load(GUSPatSynth* self, int program, int drum, FileStream* fs); /* true if success */
void	     GUSPatSynth_Unload(GUSPatSynth* self, int program, int drum);
void	     GUSPatSynth_Note(GUSPatSynth* self, int channel, int key, int velocity);
void	     GUSPatSynth_SetProgram(GUSPatSynth* self, int channel, int program, int drum);
void	     GUSPatSynth_RenderShort(GUSPatSynth* self, short* output, int frames);
void	     GUSPatSynth_RenderFloat(GUSPatSynth* self, float* output, int frames);
void	     GUSPatSynth_Destroy(GUSPatSynth* self);

#endif
