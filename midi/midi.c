#include "midi.h"

static __inline unsigned int read8(FileStream* fs) {
	unsigned char n;

	FileStream_Read(fs, &n, 1);

	return n;
}

static __inline unsigned int read16(FileStream* fs) {
	unsigned char n[2];

	FileStream_Read(fs, n, 2);

	return ((unsigned int)n[0] << 8) | n[1];
}

static __inline unsigned int read24(FileStream* fs) {
	unsigned char n[3];

	FileStream_Read(fs, n, 3);

	return ((unsigned int)n[0] << 16) | ((unsigned int)n[1] << 8) | n[2];
}

static __inline unsigned int read32(FileStream* fs) {
	unsigned char n[4];

	FileStream_Read(fs, n, 4);

	return ((unsigned int)n[0] << 24) | ((unsigned int)n[1] << 16) | ((unsigned int)n[2] << 8) | n[3];
}

static __inline unsigned int readDelta(FileStream* fs) {
	unsigned char n;
	unsigned int  r = 0;

	do {
		FileStream_Read(fs, &n, 1);

		r = r << 7;
		r = r | (n & 0x7f);
	} while(n & (1 << 7));

	return r;
}

MidiStream* MidiStream_New(FileStream* fs, MidiCallback callback) {
	MidiStream* self = calloc(1, sizeof(*self));
	MidiBigUInt toSeek;
	int	    i;

	self->fs       = fs;
	self->callback = callback;

	if(read32(self->fs) != 0x4d546864) {
		MidiStream_Destroy(self);

		return NULL;
	}

	toSeek = 8 + read32(self->fs);

	self->format = read16(self->fs);

	/* appearantly format 2 is not used a lot */
	if(self->format != 0 && self->format != 1) {
		MidiStream_Destroy(self);

		return NULL;
	}

	self->nTracks  = read16(self->fs);
	self->division = read16(self->fs);
	self->tempo    = 500000;

#ifdef DEBUG
	fprintf(stderr, "format %d, %d tracks, %d divisions\n", self->format, self->nTracks, self->division);
#endif

	FileStream_Seek(self->fs, toSeek);

	self->tracks = calloc(self->nTracks, sizeof(*self->tracks));

	for(i = 0; i < self->nTracks; i++) {
		MidiBigUInt nextTrack = 8 + FileStream_Tell(self->fs);

		if(read32(self->fs) != 0x4d54726b) {
			MidiStream_Destroy(self);

			return NULL;
		}

		self->tracks[i].dataSize  = read32(self->fs);
		self->tracks[i].nextTick  = readDelta(self->fs);
		self->tracks[i].fileStart = self->tracks[i].filePos = FileStream_Tell(self->fs);

#ifdef DEBUG
		fprintf(stderr, "track %d has %d bytes, data starts from %d, first delta is %d\n", i, self->tracks[i].dataSize, (unsigned int)self->tracks[i].fileStart, (unsigned int)self->tracks[i].nextTick);
#endif

		nextTrack += self->tracks[i].dataSize;

		FileStream_Seek(self->fs, nextTrack);
	}

	return self;
}

static void readEvent(MidiStream* self, MidiTrack* track) {
	unsigned char op;
	MidiEvent     ev;
	MidiBigUInt   oldSeek = FileStream_Tell(self->fs);

	op = read8(self->fs);

	if(!(op & (1 << 7))) {
		op = track->runningStatus;

		FileStream_Seek(self->fs, oldSeek);
	}

	switch(op & 0xf0) {
	case 0x80:
	case 0x90:
		ev.type		 = MidiEventNote;
		ev.note.channel	 = op & 0x0f;
		ev.note.key	 = read8(self->fs);
		ev.note.velocity = read8(self->fs);

		if((op & 0xf0) == 0x80) ev.note.velocity = 0;

		self->callback(self, &ev);
		break;

	case 0xa0:
		/* TODO */
		read16(self->fs);

		break;

	case 0xb0:
		/* TODO */
		read16(self->fs);

		break;

	case 0xc0:
		ev.type			 = MidiEventProgramChange;
		ev.programChange.channel = op & 0xf;
		ev.programChange.program = read8(self->fs);

		self->callback(self, &ev);
		break;

	case 0xd0:
		/* TODO */
		read8(self->fs);

		break;

	case 0xe0:
		/* TODO */
		read16(self->fs);

		break;

	case 0xf0:
		switch(op) {
		case 0xf0:
		case 0xf7:
		{
			unsigned int len = readDelta(self->fs);

			FileStream_Seek(self->fs, FileStream_Tell(self->fs) + len);
		} break;

		case 0xff:
		{
			unsigned char type = read8(self->fs);
			unsigned int  len  = readDelta(self->fs);

			switch(type) {
			case 0x2f:
				if(len == 0) {
					track->finished = 1;

					break;
				}

			case 0x51:
				if(len == 3) {
					self->tempo = read24(self->fs);

					break;
				}

			default:
				FileStream_Seek(self->fs, FileStream_Tell(self->fs) + len);
				break;
			}
		} break;
		}
		break;
	}

	if(op >= 0x80 && op <= 0xef) track->runningStatus = op;
}

void MidiStream_Advance(MidiStream* self, double sec) {
	double targetSec = self->currentSec + sec;
	double remain	 = targetSec - self->currentSec;

	while(1) {
		int	    i;
		int	    best = -1;
		MidiBigUInt tick;
		double	    deltaTick;
		double	    deltaSec;

		for(i = 0; i < self->nTracks; i++) {
			if(self->tracks[i].finished) continue;

			if(best < 0 || (self->tracks[i].nextTick < self->tracks[best].nextTick)) {
				best = i;
			}
		}

		if(best < 0) {
			self->currentSec = targetSec;

			break;
		}

		tick = self->tracks[best].nextTick;

		deltaTick = self->tracks[best].nextTick - self->currentTick;
		deltaSec  = (double)deltaTick * self->tempo / ((double)self->division * 1000000);

		/* so midi player does not get stuck */
		if(deltaSec > remain) {
			self->currentSec += remain;
			self->currentTick += remain * self->division * 1000000 / self->tempo;

			break;
		}

		remain -= deltaSec;
		self->currentSec += deltaSec;
		self->currentTick = self->tracks[best].nextTick;

		/* process all same tick events... */
		for(i = 0; i < self->nTracks; i++) {
			if(self->tracks[i].finished) continue;
			if(self->tracks[i].nextTick != tick) continue;

			FileStream_Seek(self->fs, self->tracks[i].filePos);
			readEvent(self, &self->tracks[i]);

			self->tracks[i].nextTick += readDelta(self->fs);
			self->tracks[i].filePos = FileStream_Tell(self->fs);
		}
	}
}

void MidiStream_Destroy(MidiStream* self) {
	if(self->tracks != NULL) {
		free(self->tracks);
	}

	free(self);
}
