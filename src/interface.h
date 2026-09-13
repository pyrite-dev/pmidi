#ifndef __INTERFACE_H__
#define __INTERFACE_H__

#include <turbosynth/easymidi.h>
#include <turbosynth/wavesynth.h>
#include "thread.h"

typedef struct Interface Interface;
struct Interface;

typedef struct InterfaceMod InterfaceMod;

struct InterfaceMod {
	Interface* (*New)(void);
	void (*Run)(Interface* self);
	void (*Destroy)(Interface* self);

	const char* Name;
	char	    Initial;
	const char* Description;
	int	    Continue;	/* continue after ending the midi? */
	int	    AllowEmpty; /* allow no midi? */
};

extern int    nSamples;
extern Mutex* mAudio;

extern EasyMidi*  gEasyMidi;
extern WaveSynth* gSynth;
extern int	  gPlayed;

extern InterfaceMod* iChosen;
extern Interface*    iHandle;

extern InterfaceMod* iDumb;

#endif
