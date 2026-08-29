#ifndef __TURBOSYNTH_GUSPAT_H__
#define __TURBOSYNTH_GUSPAT_H__

#include <turbosynth/fs.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct GUSSample   GUSSample;
typedef struct GUSProgram  GUSProgram;
typedef struct GUSVoice	   GUSVoice;
typedef struct GUSChannel  GUSChannel;
typedef struct GUSBank	   GUSBank;
typedef struct GUSPatSynth GUSPatSynth;

#define GUSPATSYNTH_VOICES 128
#define GUSPATSYNTH_CHANNELS 128

struct GUSSample {
	unsigned int lowFrequency;
	unsigned int highFrequency;
	unsigned int rootFrequency;

	unsigned int startLoop;
	unsigned int endLoop;

	short*	     wave;
	unsigned int nWaveFrames; /* stereo frames */
	float	     ratio;

	int loop;	  /* loop or not */
	int loopBi;	  /* 0 = uni, 1 = bi */
	int loopBackward; /* 0 = forward, 1 = backward */

	int envRate[6];
	int envOffset[6];
};

struct GUSProgram {
	int masterVolume;

	GUSSample* samples;
	int	   nSamples;

	int used;
};
typedef GUSProgram GUSProgramSet[128 * 2]; /* OR 0x80 to make it drum program */

struct GUSVoice {
	int key;

	GUSSample* sample;
	int	   volume;	  /* 16.16 */
	int	   currentVolume; /* 16.16 */
	int	   loop;

	unsigned int x;	       /* 16.16 */
	unsigned int step;     /* 16.16 */
	unsigned int baseStep; /* 16.16 */

	int used;
};

struct GUSChannel {
	int program; /* OR 0x80 to make it drum */

	int    bank;
	int    bankMsb;
	int    bankLsb;
	double pitchRatio;

	GUSVoice voices[GUSPATSYNTH_VOICES];
};

struct GUSBank {
	int	       indices[128 * 128];
	GUSProgramSet* sets;
	int	       nSets;
};

struct GUSPatSynth {
	int rate;

	GUSBank	   bank;
	GUSChannel channels[GUSPATSYNTH_CHANNELS];
};

GUSPatSynth* GUSPatSynth_New(FileStream* fs, int rate);
int	     GUSPatSynth_Load(GUSPatSynth* self, int bank, int program, int drum, FileStream* fs); /* true if success */
void	     GUSPatSynth_Unload(GUSPatSynth* self, int bank, int program, int drum);
void	     GUSPatSynth_Note(GUSPatSynth* self, int channel, int key, int velocity);
void	     GUSPatSynth_SetProgram(GUSPatSynth* self, int channel, int program, int drum);
void	     GUSPatSynth_SetBank(GUSPatSynth* self, int channel, int bank); /* immedaite change; use SetBankMSB/SetBankLSB if you are passing MIDI messages */
void	     GUSPatSynth_SetBankMSB(GUSPatSynth* self, int channel, int bank);
void	     GUSPatSynth_SetBankLSB(GUSPatSynth* self, int channel, int bank);
void	     GUSPatSynth_ChangePitchWheel(GUSPatSynth* self, int channel, double semitone);
void	     GUSPatSynth_RenderShort(GUSPatSynth* self, short* output, int frames);
void	     GUSPatSynth_RenderFloat(GUSPatSynth* self, float* output, int frames);
void	     GUSPatSynth_Reset(GUSPatSynth* self);
void	     GUSPatSynth_Destroy(GUSPatSynth* self);

#endif
