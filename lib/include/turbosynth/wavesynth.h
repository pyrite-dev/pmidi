#ifndef __TURBOSYNTH_WAVESYNTH_H__
#define __TURBOSYNTH_WAVESYNTH_H__

#include <turbosynth/fs.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct WSSample	 WSSample;
typedef struct WSProgram WSProgram;
typedef struct WSVoice	 WSVoice;
typedef struct WSChannel WSChannel;
typedef struct WSBank	 WSBank;
typedef struct WaveSynth WaveSynth;

#define WAVESYNTH_VOICES 128
#define WAVESYNTH_CHANNELS 128

struct WSSample {
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

	unsigned int envIncrement[6];
	unsigned int envOffset[6];
	int	     envEnable;
	int	     sustain;
};

struct WSProgram {
	int masterVolume;

	WSSample* samples;
	int	  nSamples;

	int used;
};
typedef WSProgram WSProgramSet[128 * 2]; /* OR 0x80 to make it drum program */

struct WSVoice {
	int key;

	WSSample* sample;
	int	  volume;	 /* 16.16 */
	int	  currentVolume; /* 16.16 */
	int	  loop;

	unsigned int x;	       /* 16.16 */
	unsigned int step;     /* 16.16 */
	unsigned int baseStep; /* 16.16 */

	int envIndex;

	int released;
	int used;
};

struct WSChannel {
	int program; /* OR 0x80 to make it drum */

	int    bank;
	int    bankMsb;
	int    bankLsb;
	double pitchRatio;

	int volume;

	WSVoice voices[WAVESYNTH_VOICES];
};

struct WSBank {
	int	      indices[128 * 128];
	WSProgramSet* sets;
	int	      nSets;
};

struct WaveSynth {
	int rate;

	WSBank	  bank;
	WSChannel channels[WAVESYNTH_CHANNELS];
};

#ifdef __cplusplus
extern "C" {
#endif

WaveSynth* WaveSynth_New(FileStream* fs, int rate);
int	   WaveSynth_Load(WaveSynth* self, int bank, int program, int drum, FileStream* fs); /* true if success */
void	   WaveSynth_Unload(WaveSynth* self, int bank, int program, int drum);
void	   WaveSynth_Note(WaveSynth* self, int channel, int key, int velocity); /* velocity is 0-127 */
void	   WaveSynth_NoteOffAll(WaveSynth* self, int channel);
void	   WaveSynth_SetBank(WaveSynth* self, int channel, int bank); /* immedaite change; use SetBankMSB/SetBankLSB if you are passing MIDI messages */
void	   WaveSynth_SetBankMSB(WaveSynth* self, int channel, int bank);
void	   WaveSynth_SetBankLSB(WaveSynth* self, int channel, int bank);
void	   WaveSynth_SetProgram(WaveSynth* self, int channel, int program, int drum);
void	   WaveSynth_SetDrum(WaveSynth* self, int channel, int drum);
void	   WaveSynth_ChangePitchWheel(WaveSynth* self, int channel, double semitone);
void	   WaveSynth_SetVolume(WaveSynth* self, int channel, double volume);
void	   WaveSynth_SetVolumeMSB(WaveSynth* self, int channel, int bank);
void	   WaveSynth_SetVolumeLSB(WaveSynth* self, int channel, int bank);
void	   WaveSynth_RenderShort(WaveSynth* self, short* output, int frames);
void	   WaveSynth_RenderFloat(WaveSynth* self, float* output, int frames);
void	   WaveSynth_Reset(WaveSynth* self);
void	   WaveSynth_Destroy(WaveSynth* self);

#ifdef __cplusplus
}
#endif

#endif
