#ifndef __TURBOSYNTH_MIDI_H__
#define __TURBOSYNTH_MIDI_H__

#include <turbosynth/fs.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdint.h>
typedef int64_t	 MidiBigInt;
typedef uint64_t MidiBigUInt;
#elif defined(__WATCOMC__)
typedef __int64		 MidiBigInt;
typedef unsigned __int64 MidiBigUInt;
#else
typedef long	      MidiBigInt;  /* :( */
typedef unsigned long MidiBigUInt; /* :( */
#endif

typedef union MidiEvent	  MidiEvent;
typedef struct MidiTrack  MidiTrack;
typedef struct MidiStream MidiStream;

typedef void (*MidiCallback)(MidiStream* ms, const MidiEvent* event);

enum MidiEventType {
	MidiEventNote = 0,
	MidiEventControl,
	MidiEventProgramChange
};

enum MidiControlType {
	MidiControlBankSelectMSB = 0,
	MidiControlBankSelectLSB = 32
};

union MidiEvent {
	unsigned char type;
	struct {
		unsigned char type;
		unsigned char channel;
		unsigned char key;
		unsigned char velocity;
	} note;
	struct {
		unsigned char type;
		unsigned char channel;
		unsigned char key;
		unsigned char value;
	} control;
	struct {
		unsigned char type;
		unsigned char channel;
		unsigned char program;
	} programChange;
};

struct MidiTrack {
	unsigned int dataSize;
	MidiBigUInt  fileStart;
	MidiBigUInt  filePos;

	MidiBigUInt nextTick;

	unsigned char runningStatus;

	int finished;
};

struct MidiStream {
	FileStream*  fs;
	MidiCallback callback;

	int format;
	int nTracks;

	unsigned int division;
	unsigned int tempo; /* microseconds per quarter note */

	double currentTick;
	double currentSec;

	MidiTrack* tracks;

	void* user;
};

MidiStream* MidiStream_New(FileStream* fs, MidiCallback callback);
void	    MidiStream_Advance(MidiStream* self, double sec);
void	    MidiStream_Destroy(MidiStream* self);

#endif
