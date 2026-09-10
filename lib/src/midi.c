#include <turbosynth/midi.h>

static __inline unsigned int read8(FileStream* fs, unsigned char** input) {
	unsigned char n;

	if(input == NULL) {
		FileStream_Read(fs, &n, 1);
	} else {
		n = **input;
		(*input) += 1;
	}

	return n;
}

static __inline unsigned int read16(FileStream* fs, unsigned char** input) {
	unsigned char n[2];

	if(input == NULL) {
		FileStream_Read(fs, n, 2);
	} else {
		memcpy(n, *input, 2);
		(*input) += 2;
	}

	return ((unsigned int)n[0] << 8) | n[1];
}

static __inline unsigned int read24(FileStream* fs, unsigned char** input) {
	unsigned char n[3];

	if(input == NULL) {
		FileStream_Read(fs, n, 3);
	} else {
		memcpy(n, *input, 3);
		(*input) += 3;
	}

	return ((unsigned int)n[0] << 16) | ((unsigned int)n[1] << 8) | n[2];
}

static __inline unsigned int read32(FileStream* fs, unsigned char** input) {
	unsigned char n[4];

	if(input == NULL) {
		FileStream_Read(fs, n, 4);
	} else {
		memcpy(n, *input, 4);
		(*input) += 4;
	}

	return ((unsigned int)n[0] << 24) | ((unsigned int)n[1] << 16) | ((unsigned int)n[2] << 8) | n[3];
}

static __inline unsigned int readDelta(FileStream* fs, unsigned char** input) {
	unsigned char n;
	unsigned int  r = 0;

	do {
		if(input == NULL) {
			if(FileStream_Read(fs, &n, 1) < 1) break;
		} else {
			n = **input;
			(*input)++;
		}

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

	if(read32(self->fs, NULL) != 0x4d546864) {
		MidiStream_Destroy(self);

		return NULL;
	}

	toSeek = 8 + read32(self->fs, NULL);

	self->format = read16(self->fs, NULL);

	/* appearantly format 2 is not used a lot */
	if(self->format != 0 && self->format != 1) {
		MidiStream_Destroy(self);

		return NULL;
	}

	self->nTracks  = read16(self->fs, NULL);
	self->division = read16(self->fs, NULL);
	self->tempo    = 500000;

#ifdef DEBUG
	fprintf(stderr, "format %d, %d tracks, %d divisions\n", self->format, self->nTracks, self->division);
#endif

	FileStream_Seek(self->fs, toSeek);

	self->tracks = calloc(self->nTracks, sizeof(*self->tracks));

	for(i = 0; i < self->nTracks; i++) {
		MidiBigUInt nextTrack = 8 + FileStream_Tell(self->fs);

		if(read32(self->fs, NULL) != 0x4d54726b) {
			MidiStream_Destroy(self);

			return NULL;
		}

		self->tracks[i].dataSize  = read32(self->fs, NULL);
		self->tracks[i].nextTick  = readDelta(self->fs, NULL);
		self->tracks[i].fileStart = self->tracks[i].filePos = FileStream_Tell(self->fs);

#ifdef DEBUG
		fprintf(stderr, "track %d has %d bytes, data starts from %d, first delta is %d\n", i, self->tracks[i].dataSize, (unsigned int)self->tracks[i].fileStart, (unsigned int)self->tracks[i].nextTick);
#endif

		nextTrack += self->tracks[i].dataSize;

		FileStream_Seek(self->fs, nextTrack);
	}

	return self;
}

void MidiStream_Parse(FileStream* fs, unsigned char** buf, MidiTrack* track, MidiEvent* ev) {
	MidiBigUInt   oldSeek = buf != NULL ? 0 : FileStream_Tell(fs);
	unsigned char op;

	ev->type = 0;

	op = read8(fs, buf);

	if(track != NULL && !(op & (1 << 7))) {
		op = track->runningStatus;

		if(buf == NULL) {
			FileStream_Seek(fs, oldSeek);
		} else {
			(*buf)--;
		}
	}

	switch(op & 0xf0) {
	case 0x80:
	case 0x90:
		ev->type	  = MidiEventNote;
		ev->note.channel  = op & 0x0f;
		ev->note.key	  = read8(fs, buf);
		ev->note.velocity = read8(fs, buf);

		if((op & 0xf0) == 0x80) ev->note.velocity = 0;
		break;

	case 0xa0:
		/* TODO */
		read16(fs, buf);

		break;

	case 0xb0:
		ev->type	    = MidiEventControl;
		ev->control.channel = op & 0x0f;
		ev->control.key	    = read8(fs, buf);
		ev->control.value   = read8(fs, buf);
		break;

	case 0xc0:
		ev->type		  = MidiEventProgramChange;
		ev->programChange.channel = op & 0xf;
		ev->programChange.program = read8(fs, buf);
		break;

	case 0xd0:
		/* TODO */
		read8(fs, buf);

		break;

	case 0xe0:
	{
		int bendl = read8(fs, buf);
		int bendm = read8(fs, buf);

		ev->type		      = MidiEventPitchWheelChange;
		ev->pitchWheelChange.channel  = op & 0xf;
		ev->pitchWheelChange.bend     = ((bendm << 7) | bendm) - 8192;
		ev->pitchWheelChange.semitone = (double)ev->pitchWheelChange.bend / 8192 * 2;
		break;
	}

	case 0xf0:
		switch(op) {
		case 0xf0:
		case 0xf7:
		{
			unsigned int len = readDelta(fs, buf);
			int	     i;

			for(i = 0; i < len; i++) read8(fs, buf);
			break;
		}

		case 0xff:
		{
			unsigned char type = read8(fs, buf);
			unsigned int  len  = readDelta(fs, buf);
			int	      i;

			switch(type) {
			case 0x2f:
				if(len == 0 && track != NULL) {
					track->finished = 1;
				}
				break;

			case 0x51:
				if(len == 3) {
					ev->tempoChange.type  = MidiEventTempoChange;
					ev->tempoChange.tempo = read24(fs, buf);
				}
				break;

			default:
				for(i = 0; i < len; i++) read8(fs, buf);
				break;
			}
			break;
		}
		}
		break;
	}

	if(track != NULL && op >= 0x80 && op <= 0xef) track->runningStatus = op;
}

static void readEvent(MidiStream* self, MidiTrack* track) {
	MidiEvent ev;

	MidiStream_Parse(self->fs, NULL, track, &ev);

	if(ev.type != 0) {
		if(ev.type == MidiEventTempoChange) self->tempo = ev.tempoChange.tempo;

		self->callback(self, &ev);
	}
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

			if(FileStream_Tell(self->fs) - self->tracks[i].fileStart >= self->tracks[i].dataSize) {
				self->tracks[i].finished = 1;
				continue;
			}

			self->tracks[i].nextTick += readDelta(self->fs, NULL);
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
