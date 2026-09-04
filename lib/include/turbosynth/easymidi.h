#ifndef __TURBOSYNTH_EASYMIDI_H__
#define __TURBOSYNTH_EASYMIDI_H__

#include <turbosynth/fs.h>
#include <turbosynth/midi.h>
#include <turbosynth/wavesynth.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct EasyMidi EasyMidi;

struct EasyMidi {
	int rate;

	MidiStream* ms;
	FileStream* fs;

	WaveSynth* synth;
};

#ifdef __cplusplus
extern "C" {
#endif

EasyMidi* EasyMidi_New(const char* cfg, int rate);
EasyMidi* EasyMidi_New2(FileStream* cfg, int rate);
void	  EasyMidi_MidiCallback(MidiStream* ms, const MidiEvent* event); /* you may use this with self->user pointed at WaveSynth* */
int	  EasyMidi_Load(EasyMidi* self, const char* midi);
int	  EasyMidi_Load2(EasyMidi* self, FileStream* midi);
int	  EasyMidi_IsFinished(EasyMidi* self);
void	  EasyMidi_RenderShort(EasyMidi* self, short* frames, int nFrames);
void	  EasyMidi_RenderFloat(EasyMidi* self, float* frames, int nFrames);
void	  EasyMidi_Reset(EasyMidi* self);
void	  EasyMidi_Destroy(EasyMidi* self);

#ifdef __cplusplus
}
#endif

#endif
