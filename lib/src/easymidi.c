#include <turbosynth/easymidi.h>

EasyMidi* EasyMidi_New(const char* cfg, int rate) {
	FileStream* fs;
	EasyMidi*   self;

	if((fs = FileStream_New(cfg, NULL)) == NULL) return NULL;

	self = EasyMidi_New2(fs, rate);

	FileStream_Destroy(fs);

	return self;
}

EasyMidi* EasyMidi_New2(FileStream* cfg, int rate) {
	EasyMidi* self = calloc(1, sizeof(*self));

	self->rate = rate;

	if((self->synth = GUSPatSynth_New(cfg, rate)) == NULL) {
		free(self);

		return NULL;
	}

	return self;
}

void EasyMidi_MidiCallback(MidiStream* ms, const MidiEvent* event) {
	GUSPatSynth* synth = ms->user;

	if(event->type == MidiEventNote) {
		GUSPatSynth_Note(synth, event->note.channel, event->note.key, event->note.velocity);
	} else if(event->type == MidiEventControl) {
		if(event->control.key == MidiControlBankSelectMSB) {
			GUSPatSynth_SetBankMSB(synth, event->control.channel, event->control.value);
		} else if(event->control.key == MidiControlBankSelectLSB) {
			GUSPatSynth_SetBankLSB(synth, event->control.channel, event->control.value);
		}
	} else if(event->type == MidiEventProgramChange) {
		int drum = 0;

		if(synth->channels[event->programChange.channel].bankMsb == 120 || event->programChange.channel == 9) drum = 1;

		GUSPatSynth_SetProgram(synth, event->programChange.channel, event->programChange.program, drum);
	}
}

int EasyMidi_Load(EasyMidi* self, const char* midi) {
	FileStream* fs;
	int	    s;

	if((fs = FileStream_New(midi, NULL)) == NULL) return 0;

	s = EasyMidi_Load2(self, fs);

	if(s == 0) {
		FileStream_Destroy(self->fs);
		self->fs = NULL;
	}

	return s;
}

int EasyMidi_Load2(EasyMidi* self, FileStream* midi) {
	if(self->ms != NULL) MidiStream_Destroy(self->ms);
	if(self->fs != NULL) FileStream_Destroy(self->fs);

	self->fs = midi;
	self->ms = NULL;

	if((self->ms = MidiStream_New(self->fs, EasyMidi_MidiCallback)) == NULL) return 0;

	self->ms->user = self->synth;

	return 1;
}

int EasyMidi_IsFinished(EasyMidi* self) {
	int i;

	if(self->ms == NULL) return 0;

	for(i = 0; i < self->ms->nTracks && self->ms->tracks[i].finished; i++);
	if(i == self->ms->nTracks) return 1;

	return 0;
}

void EasyMidi_RenderShort(EasyMidi* self, short* frames, int nFrames) {
	if(self->ms == NULL) return;

	MidiStream_Advance(self->ms, (double)nFrames / self->rate);
	GUSPatSynth_RenderShort(self->synth, frames, nFrames);
}

void EasyMidi_RenderFloat(EasyMidi* self, float* frames, int nFrames) {
	if(self->ms == NULL) return;

	MidiStream_Advance(self->ms, (double)nFrames / self->rate);
	GUSPatSynth_RenderFloat(self->synth, frames, nFrames);
}

void EasyMidi_Destroy(EasyMidi* self) {
	if(self->ms != NULL) MidiStream_Destroy(self->ms);
	if(self->fs != NULL) FileStream_Destroy(self->fs);
	GUSPatSynth_Destroy(self->synth);
	free(self);
}
