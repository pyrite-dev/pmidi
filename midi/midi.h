#ifndef __MIDI_H__
#define __MIDI_H__

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

typedef struct MidiStream MidiStream;
typedef union MidiEvent	  MidiEvent;
typedef struct MidiTrack  MidiTrack;
typedef struct FileStream FileStream;

typedef void (*MidiCallback)(MidiStream* ms, const MidiEvent* event);

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
};

enum MidiEventType {
	MidiEventNote = 0,
	MidiEventProgramChange
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

struct FileStream {
	int (*Read)(FileStream* self, void* buf, int size);
	void (*Seek)(FileStream* self, MidiBigUInt pos);
	MidiBigUInt (*Tell)(FileStream* self);
	void (*Close)(FileStream* self);
};

MidiStream* MidiStream_New(FileStream* fs, MidiCallback callback);
void	    MidiStream_Advance(MidiStream* self, double sec);
void	    MidiStream_Destroy(MidiStream* self);

FileStream* FileStream_New(const char* path);
#define FileStream_Read(self, buf, size) (self)->Read((self), (buf), (size))
#define FileStream_Seek(self, pos) (self)->Seek((self), (pos))
#define FileStream_Tell(self) (self)->Tell((self))
#define FileStream_Close(self) (self)->Close((self))
void FileStream_Destroy(FileStream* self);

#endif
