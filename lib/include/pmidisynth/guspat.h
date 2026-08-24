#ifndef __PMIDISYNTH_GUSPAT_H__
#define __PMIDISYNTH_GUSPAT_H__

#include <pmidisynth/fs.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct GUSSample   GUSSample;
typedef struct GUSProgram  GUSProgram;
typedef struct GUSVoice	   GUSVoice;
typedef struct GUSChannel  GUSChannel;
typedef struct GUSPatSynth GUSPatSynth;

#define GUSPATSYNTH_VOICES 128
#define GUSPATSYNTH_CHANNELS 128

struct GUSSample {
	unsigned int lowFrequency;
	unsigned int highFrequency;
	unsigned int rootFrequency;

	unsigned int startLoop;
	unsigned int endLoop;

	short*	     wave;	  /* stereo interleaved @ self->rate Hz */
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

	GUSSample*   sample;
	unsigned int volume; /* 16.16 */
	int	     loop;

	unsigned int x;	   /* 16.16 */
	unsigned int step; /* 16.16 */

	int used;
};

struct GUSChannel {
	int program; /* OR 0x80 to make it drum */

	GUSVoice voices[GUSPATSYNTH_VOICES];
};

struct GUSPatSynth {
	int rate;

	GUSProgram programs[128 * 2]; /* OR 0x80 to make it drum program */
	GUSChannel channels[GUSPATSYNTH_CHANNELS];
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
