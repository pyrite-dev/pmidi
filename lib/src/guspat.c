#include <turbosynth/guspat.h>

#define LINESZ 1024
#define TOL 0

#define RESAMPLE_ON_PLAY
#define MONAURAL

#ifdef MONAURAL
#define CHANNELS 1
#define CH1 0
#define CH2 0
#else
#define CHANNELS 2
#define CH1 0
#define CH2 1
#endif

static const int freqTable[128] = {
    8176, 8662, 9177, 9723,
    10301, 10913, 11562, 12250,
    12978, 13750, 14568, 15434,

    16352, 17324, 18354, 19445,
    20602, 21827, 23125, 24500,
    25957, 27500, 29135, 30868,

    32703, 34648, 36708, 38891,
    41203, 43654, 46249, 48999,
    51913, 55000, 58270, 61735,

    65406, 69296, 73416, 77782,
    82407, 87307, 92499, 97999,
    103826, 110000, 116541, 123471,

    130813, 138591, 146832, 155563,
    164814, 174614, 184997, 195998,
    207652, 220000, 233082, 246942,

    261626, 277183, 293665, 311127,
    329628, 349228, 369994, 391995,
    415305, 440000, 466164, 493883,

    523251, 554365, 587330, 622254,
    659255, 698456, 739989, 783991,
    830609, 880000, 932328, 987767,

    1046502, 1108731, 1174659, 1244508,
    1318510, 1396913, 1479978, 1567982,
    1661219, 1760000, 1864655, 1975533,

    2093005, 2217461, 2349318, 2489016,
    2637020, 2793826, 2959955, 3135963,
    3322438, 3520000, 3729310, 3951066,

    4186009, 4434922, 4698636, 4978032,
    5274041, 5587652, 5919911, 6271927,
    6644875, 7040000, 7458620, 7902133,

    8372018, 8869844, 9397273, 9956063,
    10548082, 11175303, 11839822, 12543854};

static __inline unsigned int read8(FileStream* fs) {
	unsigned char n;

	FileStream_Read(fs, &n, 1);

	return n;
}

static __inline unsigned int read16(FileStream* fs) {
	unsigned char n[2];

	FileStream_Read(fs, n, 2);

	return ((unsigned int)n[1] << 8) | n[0];
}

static __inline unsigned int read24(FileStream* fs) {
	unsigned char n[3];

	FileStream_Read(fs, n, 3);

	return ((unsigned int)n[2] << 16) | ((unsigned int)n[1] << 8) | n[0];
}

static __inline unsigned int read32(FileStream* fs) {
	unsigned char n[4];

	FileStream_Read(fs, n, 4);

	return ((unsigned int)n[3] << 24) | ((unsigned int)n[2] << 16) | ((unsigned int)n[1] << 8) | n[0];
}

static unsigned int keyFrequency(int note) {
	if(note < 0 || note >= 128) return 0;

	return freqTable[note];
}

GUSPatSynth* GUSPatSynth_New(FileStream* fs, int rate) {
	GUSPatSynth* self = calloc(1, sizeof(*self));
	char	     line[LINESZ + 1];
	char	     c[2];
	int	     comment = 0;
	int	     num     = -1; /* -1 to ignore numbers */

	self->rate = rate;

	if(fs == NULL) return self;

	line[0] = 0;
	c[1]	= 0;

	while(1) {
		int n;

		if((n = FileStream_Read(fs, c, 1)) == 0 || c[0] == '\n') {
			char* arg0 = line;
			char* arg1 = line;

			while(*arg1 != 0 && *arg1 != ' ' && *arg1 != '\t') arg1++;
			if(*arg1) {
				*arg1++ = 0;

				while(*arg1 != 0 && (*arg1 == ' ' || *arg1 == '\t')) arg1++;

				if(*arg1) {
					if(*arg1 == '"' || *arg1 == '\'') arg1++;
					if(arg1[strlen(arg1) - 1] == '"' || arg1[strlen(arg1) - 1] == '\'') arg1[strlen(arg1)] = 0;

					if((num != -1) && '0' <= arg0[0] && arg0[0] <= '9') {
						int program = atoi(arg0);

						if(0 <= program && program < 128) {
							FileStream* patch;
							int	    p2l	       = strlen(fs->path == NULL ? "" : fs->path) + 10 + strlen(arg1) + 4 + 1;
							char*	    patchpath  = malloc(strlen(arg1) + 4 + 1);
							char*	    patchpath2 = malloc(p2l);
							char*	    patchpath3 = malloc(p2l);

							strcpy(patchpath, arg1);
							strcat(patchpath, ".pat");

							if(fs->path == NULL) {
								patchpath2[0] = patchpath3[0] = 0;
							} else {
								char* n;

								strcpy(patchpath2, fs->path);
								if((n = strrchr(patchpath2, '/')) == NULL) {
									strcpy(patchpath2, "./");
								} else {
									n[0] = 0;
									for(n--; n != patchpath2 && *n == '/' && *n == '\\'; n--) n[0] = 0;
									strcat(patchpath2, "/");
								}
								strcat(patchpath2, arg1);

								strcpy(patchpath3, patchpath2);
								strcat(patchpath3, ".pat");
							}

							if((patch = fs->New(arg1, fs->newArg)) != NULL || (patch = fs->New(patchpath, fs->newArg)) != NULL || (patch = fs->New(patchpath2, fs->newArg)) != NULL || (patch = fs->New(patchpath3, fs->newArg)) != NULL) {
								if(!GUSPatSynth_Load(self, num & 0xff, program, num & (1 << 8), patch)) {
									FileStream_Destroy(patch);
									free(patchpath3);
									free(patchpath2);
									free(patchpath);
									GUSPatSynth_Destroy(self);

									return NULL;
								}

								FileStream_Destroy(patch);
								free(patchpath3);
								free(patchpath2);
								free(patchpath);
							} else {
								free(patchpath3);
								free(patchpath2);
								free(patchpath);
								GUSPatSynth_Destroy(self);

								return NULL;
							}
						}
					} else if(strcmp(arg0, "bank") == 0 || strcmp(arg0, "drumset") == 0) {
						num = atoi(arg1) | (strcmp(arg0, "drumset") == 0 ? (1 << 8) : 0);

						if(self->bank.indices[num & 0xff] == 0) {
							self->bank.indices[num & 0xff] = self->bank.nSets + 1;

							self->bank.sets = self->bank.nSets == 0 ? malloc(sizeof(*self->bank.sets)) : realloc(self->bank.sets, sizeof(*self->bank.sets) * (self->bank.nSets + 1));
							memset(self->bank.sets[self->bank.nSets], 0, sizeof(*self->bank.sets));

							self->bank.nSets++;
						}
					}
				}
			}

			line[0] = 0;
			comment = 0;

			if(n == 0) break;
		} else if((strlen(line) > 0 || (c[0] != ' ' && c[0] != '\t')) && c[0] != '\r' && strlen(line) < LINESZ) {
			if(c[0] == '#') {
				int i;

				comment = 1;

				for(i = strlen(line) - 1; i >= 0 && (line[i] == ' ' || line[i] == '\t'); i--) line[i] = 0;
			} else if(!comment) {
				strcat(line, c);
			}
		}
	}

	GUSPatSynth_Reset(self);

	return self;
}

static void loadSample(GUSSample* sample, FileStream* fs, int patchChannels, int rate) {
	unsigned int	waveSize;
	int		balance;
	unsigned int	sampleRate;
	int		modes;
	void*		wave;
	int		i;
	signed char*	s8;
	unsigned char*	u8;
	short*		s16;
	unsigned short* u16;
	unsigned char	envRate[6];
	unsigned char	envOffset[6];

	FileStream_Seek(fs, FileStream_Tell(fs) + 7); /* wave name */
	read8(fs);				      /* fractions */
	waveSize	      = read32(fs);
	sample->startLoop     = read32(fs);
	sample->endLoop	      = read32(fs);
	sampleRate	      = read16(fs);
	sample->lowFrequency  = read32(fs);
	sample->highFrequency = read32(fs);
	sample->rootFrequency = read32(fs);
	read16(fs); /* tune */
	balance = ((float)read8(fs) - 128) / 128;
	FileStream_Read(fs, envRate, 6);
	FileStream_Read(fs, envOffset, 6);
	read8(fs); /* tremolo sweep */
	read8(fs); /* tremolo rate */
	read8(fs); /* tremolo depth */
	read8(fs); /* vibrato sweep */
	read8(fs); /* vibrato rate */
	read8(fs); /* vibrato depth */
	modes = read8(fs);
	read16(fs);				       /* scale frequency */
	read16(fs);				       /* scale factor */
	FileStream_Seek(fs, FileStream_Tell(fs) + 36); /* reserved */

	sample->loop	     = (modes & (1 << 2)) ? 1 : 0;
	sample->loopBi	     = (modes & (1 << 3)) ? 1 : 0;
	sample->loopBackward = (modes & (1 << 4)) ? 1 : 0;

	sample->startLoop = sample->startLoop / ((modes & 1) ? 2 : 1) / (patchChannels == 2 ? 2 : 1);
	sample->endLoop	  = sample->endLoop / ((modes & 1) ? 2 : 1) / (patchChannels == 2 ? 2 : 1);

#ifdef RESAMPLE_ON_PLAY
	sample->ratio = (float)sampleRate / rate;
#else
	sample->ratio = 1;

	sample->startLoop = sample->startLoop * rate / sampleRate;
	sample->endLoop	  = sample->endLoop * rate / sampleRate;
#endif

	for(i = 0; i < 6; i++) {
		sample->envIncrement[i] = ((unsigned int)(envRate[i] & 0x3f) << 16) / 4095;
		sample->envOffset[i]	= ((unsigned int)envOffset[i] << 16) / 255;
	}

	sample->envEnable = (modes & (1 << 6)) ? 1 : 0;
	sample->sustain	  = (modes & (1 << 5)) ? 1 : 0;

#ifdef DEBUG
	fprintf(stderr, "new sample, wave size is %d, start loop is %d, end loop is %d, sample rate is %d, low freq is %d, high freq is %d, root freq is %d\n", waveSize, sample->startLoop, sample->endLoop, sampleRate, sample->lowFrequency, sample->highFrequency, sample->rootFrequency);
#endif

	wave = malloc(waveSize);
	FileStream_Read(fs, wave, waveSize);

	s8  = wave;
	s16 = wave;
	u8  = wave;
	u16 = wave;

	if(modes & 1) {
		for(i = 0; i < waveSize; i += 2) {
			u16[i / 2] = ((int)u8[i + 1] << 8) | u8[i + 0];
		}
	}

	if(modes & 1) waveSize /= 2;
	if(patchChannels == 2) waveSize /= 2;

#ifdef RESAMPLE_ON_PLAY
	sample->nWaveFrames = waveSize;
#else
	sample->nWaveFrames = waveSize * rate / sampleRate;
#endif

#ifdef MONAURAL
	sample->wave = calloc(sample->nWaveFrames, sizeof(*sample->wave));
#else
	sample->wave = calloc(sample->nWaveFrames * 2, sizeof(*sample->wave));
#endif

	for(i = 0; i < sample->nWaveFrames; i++) {
		float fl, fr;
#ifdef RESAMPLE_ON_PLAY
		int from = i;
#else
		int from = i * sampleRate / rate;
#endif

		switch(modes & 3) {
		case 0: /* S8 */
			fl = s8[from * patchChannels + 0] / 128.0;
			if(patchChannels == 2) {
				fr = s8[from * patchChannels + 1] / 128.0;
			} else {
				fr = fl;
			}
			break;

		case 1: /* S16 */
			fl = s16[from * patchChannels + 0] / 32768.0;
			if(patchChannels == 2) {
				fr = s16[from * patchChannels + 1] / 32768.0;
			} else {
				fr = fl;
			}
			break;

		case 2: /* U8 */
			fl = ((float)u8[from * patchChannels + 0] - 128) / 128.0;
			if(patchChannels == 2) {
				fr = ((float)u8[from * patchChannels + 1] - 128) / 128.0;
			} else {
				fr = fl;
			}
			break;

		case 3: /* U16 */
			fl = ((float)u16[from * patchChannels + 0] - 32768) / 32768.0;
			if(patchChannels == 2) {
				fr = ((float)u16[from * patchChannels + 1] - 32768) / 32768.0;
			} else {
				fr = fl;
			}
			break;
		}

#ifdef MONAURAL
		sample->wave[i] = (fl + fr) / 2 * 32767;
#else
		sample->wave[i * 2 + 0] = fl * 32767;
		sample->wave[i * 2 + 1] = fr * 32767;
#endif
	}

	free(wave);
}

static int loadPatch(GUSProgram* prog, FileStream* fs, int rate) {
	char buffer[64]; /* enough for temporary buffer */
	int  i, j;
	int  instruments;
	int  channels;
	int  layers;

	FileStream_Read(fs, buffer, 22);
	if(memcmp(buffer, "GF1PATCH100\0ID#000002\0", 22) != 0 && memcmp(buffer, "GF1PATCH110\0ID#000002\0", 22) != 0) {
		return 0;
	}

	FileStream_Seek(fs, FileStream_Tell(fs) + 60); /* description */
	instruments = read8(fs);
	read8(fs); /* voices */
	channels = read8(fs);
	read16(fs);				       /* waveforms */
	prog->masterVolume = read16(fs);	       /* doc says this is unused */
	read32(fs);				       /* data size */
	FileStream_Seek(fs, FileStream_Tell(fs) + 36); /* reserved */

	if(instruments == 0) instruments = 1;
	if(channels == 0) channels = 1;

#ifdef DEBUG
	fprintf(stderr, "new patch, %d instruments, %d channels, master volume %d\n", instruments, channels, prog->masterVolume);
#endif

	if(instruments != 1) {
		return 0;
	}

	read16(fs);				       /* instrument */
	FileStream_Seek(fs, FileStream_Tell(fs) + 16); /* instrument name */
	read32(fs);				       /* instrument size */
	layers = read8(fs);
	FileStream_Seek(fs, FileStream_Tell(fs) + 40); /* reserved */

#ifdef DEBUG
	fprintf(stderr, "new instrument, %d layers\n", layers);
#endif

	for(i = 0; i < layers; i++) {
		int samples;

		read8(fs);  /* layer duplicate */
		read8(fs);  /* layer */
		read32(fs); /* layer size */
		samples = read8(fs);
		FileStream_Seek(fs, FileStream_Tell(fs) + 40); /* reserved */

#ifdef DEBUG
		fprintf(stderr, "new layer, %d samples\n", samples);
#endif

		prog->samples  = calloc(samples, sizeof(*prog->samples));
		prog->nSamples = samples;

		for(j = 0; j < samples; j++) loadSample(&prog->samples[j], fs, channels, rate);
	}

	prog->used = 1;

	return 1;
}

static void unloadPatch(GUSProgram* prog) {
	prog->used = 0;

	if(prog->samples != NULL) {
		int i;

		for(i = 0; i < prog->nSamples; i++) {
			free(prog->samples[i].wave);
		}

		free(prog->samples);
		prog->samples = NULL;
	}
}

static GUSProgramSet* getProgramSet(GUSPatSynth* self, int bank) {
	if(!self->bank.indices[bank]) return NULL;

	return &self->bank.sets[self->bank.indices[bank] - 1];
}

int GUSPatSynth_Load(GUSPatSynth* self, int bank, int program, int drum, FileStream* fs) {
	GUSProgramSet* ps = getProgramSet(self, bank);

	if((*ps)[(drum ? 0x80 : 0) | program].used) GUSPatSynth_Unload(self, bank, program, drum);
	return loadPatch(&(*ps)[(drum ? 0x80 : 0) | program], fs, self->rate);
}

void GUSPatSynth_Unload(GUSPatSynth* self, int bank, int program, int drum) {
	GUSProgramSet* ps = getProgramSet(self, bank);

	unloadPatch(&(*ps)[(drum ? 0x80 : 0) | program]);
}

#define NOTE_OFF(voice) \
	{ \
		if(voice->sample->envEnable) { \
			voice->released = 1; \
		} else { \
			voice->used = 0; \
		} \
		voice->envIndex = 2; \
	}

void GUSPatSynth_Note(GUSPatSynth* self, int channel, int key, int velocity) {
	int i;

	if(velocity == 0) {
		for(i = 0; i < GUSPATSYNTH_VOICES; i++) {
			GUSVoice* voice = &self->channels[channel].voices[i];
			if(!voice->used || voice->released) continue;

			if(voice->key == key) {
				NOTE_OFF(voice);
			}
		}
	} else {
		GUSVoice* voice;

		for(i = 0; i < GUSPATSYNTH_VOICES && (voice = &self->channels[channel].voices[i])->used; i++);

		if(i < GUSPATSYNTH_VOICES) {
			int	       drum = self->channels[channel].program >= 0x80;
			int	       bank = self->channels[channel].bank;
			GUSProgramSet* ps;
			GUSProgram*    prog;

		retry:;
			ps   = getProgramSet(self, bank);
			prog = &(*ps)[drum ? (0x80 | key) : self->channels[channel].program];

			if(bank != 0 && !prog->used) {
				bank = 0;
				goto retry;
			}

			voice->key	= key;
			voice->sample	= NULL;
			voice->x	= 0;
			voice->step	= 0;
			voice->volume	= (float)velocity / 127 / 4 * 32768;
			voice->envIndex = 0;
			voice->released = 0;

			if(prog->used) {
				int freq = keyFrequency(key);

				for(i = 0; i < prog->nSamples; i++) {
					GUSSample* sample = &prog->samples[i];

					if(drum || (sample->lowFrequency <= (freq + TOL) && freq <= (sample->highFrequency + TOL))) {
						voice->sample	     = sample;
						voice->baseStep	     = (unsigned int)((double)(drum ? sample->rootFrequency : freq) / sample->rootFrequency * sample->ratio * 65536);
						voice->loop	     = sample->loop && !sample->loopBi && !sample->loopBackward;
						voice->currentVolume = sample->envEnable ? sample->envOffset[0] : 0x10000;

						voice->step = voice->baseStep * self->channels[channel].pitchRatio;
					}
				}
			}

			if(voice->sample != NULL) {
				voice->used = 1;
			}

#ifdef DEBUG
			if(!voice->sample) {
				fprintf(stderr, "bank %d program %d drum %s\n", self->channels[channel].bank, self->channels[channel].program >= 0x80 ? key : self->channels[channel].program, self->channels[channel].program >= 0x80 ? "true" : "false");
			}
#endif
		}
	}
}

void GUSPatSynth_NoteOffAll(GUSPatSynth* self, int key) {
	int i;

	for(i = 0; i < GUSPATSYNTH_VOICES; i++) {
		GUSVoice* voice = &self->channels[channel].voices[i];
		if(!voice->used || voice->released) continue;

		NOTE_OFF(voice);
	}
}

void GUSPatSynth_SetProgram(GUSPatSynth* self, int channel, int program, int drum) {
	int bank = (self->channels[channel].bankMsb << 7) | self->channels[channel].bankLsb;

	GUSPatSynth_SetBank(self, channel, bank);
	self->channels[channel].program = (drum ? 0x80 : 0) | (program & 0x7f);
}

void GUSPatSynth_SetBank(GUSPatSynth* self, int channel, int bank) {
	if(getProgramSet(self, bank) != NULL) self->channels[channel].bank = bank;
	self->channels[channel].bankMsb = (bank >> 7) & 0x7f;
	self->channels[channel].bankLsb = bank & 0x7f;
}

void GUSPatSynth_SetBankMSB(GUSPatSynth* self, int channel, int bank) {
	self->channels[channel].bankMsb = bank & 0x7f;
}

void GUSPatSynth_SetBankLSB(GUSPatSynth* self, int channel, int bank) {
	self->channels[channel].bankLsb = bank & 0x7f;
}

void GUSPatSynth_ChangePitchWheel(GUSPatSynth* self, int channel, double semitone) {
	int i;

	self->channels[channel].pitchRatio = pow(2, semitone / 12);

	for(i = 0; i < GUSPATSYNTH_VOICES; i++) {
		self->channels[channel].voices[i].step = self->channels[channel].voices[i].baseStep * self->channels[channel].pitchRatio;
	}
}

#define ENVELOPE \
	{ \
		int rel2 = voice->released && voice->envIndex == 2; \
		int b	 = rel2 ? sample->envOffset[3] : sample->envOffset[voice->envIndex + 1]; \
		int inc	 = sample->envIncrement[voice->envIndex]; \
\
		if(voice->currentVolume < b) { \
			voice->currentVolume += inc; \
			if(voice->currentVolume >= b) { \
				voice->currentVolume = b; \
			} \
		} else if(voice->currentVolume > b) { \
			voice->currentVolume -= inc; \
			if(voice->currentVolume <= b) { \
				voice->currentVolume = b; \
			} \
		} else { \
			voice->currentVolume = b; \
		} \
\
		if(voice->currentVolume == b && (voice->released || !sample->sustain || voice->envIndex != 1)) voice->envIndex++; \
		if(voice->envIndex == 5) { \
			voice->used = 0; \
		} \
	}

#define RENDER(proc) \
	int i, j, k, l; \
\
	memset(output, 0, frames * 2 * sizeof(*output)); \
\
	for(i = 0; i < GUSPATSYNTH_CHANNELS; i++) { \
		GUSChannel* channel = &self->channels[i]; \
\
		for(j = 0; j < GUSPATSYNTH_VOICES; j++) { \
			GUSVoice*  voice  = &channel->voices[j]; \
			GUSSample* sample = voice->sample; \
\
			if(!voice->used) continue; \
\
			for(k = 0; k < frames; k++) { \
				int    x    = voice->x >> 16; \
				short* wave = &sample->wave[x * CHANNELS]; \
\
				proc; \
\
				voice->x += voice->step; \
\
				/* TODO: implement more than forward loop */ \
				if(voice->loop && x >= sample->endLoop) { \
					voice->x = (sample->startLoop + (x - sample->endLoop)) << 16; \
				} else if(!sample->loop && x >= sample->nWaveFrames) { \
					voice->used = 0; \
				} \
\
				if(voice->used && sample->envEnable) ENVELOPE; \
\
				if(!voice->used) break; \
				if(x >= sample->nWaveFrames) voice->x = (sample->nWaveFrames - 1) << 16; \
			} \
		} \
	}

void GUSPatSynth_RenderShort(GUSPatSynth* self, short* output, int frames) {
	int* mix = calloc(frames * 2, sizeof(*mix));

	RENDER({
		mix[k * 2 + 0] += ((((int)wave[CH1] * voice->volume) >> 16) * voice->currentVolume) >> 16;
		mix[k * 2 + 1] += ((((int)wave[CH2] * voice->volume) >> 16) * voice->currentVolume) >> 16;
	});

	for(i = 0; i < frames * 2; i++) {
		int n = mix[i];

		if(n < -32767) n = -32767;
		if(n > 32767) n = 32767;

		output[i] = n;
	}

	free(mix);
}

void GUSPatSynth_RenderFloat(GUSPatSynth* self, float* output, int frames) {
	memset(output, 0, frames * 2 * sizeof(*output));

	RENDER({
		output[k * 2 + 0] += (float)wave[CH1] / 32767 * (voice->volume / 32768.0) * (voice->currentVolume / 32768.0);
		output[k * 2 + 1] += (float)wave[CH2] / 32767 * (voice->volume / 32768.0) * (voice->currentVolume / 32768.0);
	});

	for(i = 0; i < frames * 2; i++) {
		float n = output[i];

		if(n < -1) n = -1;
		if(n > 1) n = 1;

		output[i] = n;
	}
}

void GUSPatSynth_Reset(GUSPatSynth* self) {
	int i;

	memset(self->channels, 0, sizeof(self->channels));

	for(i = 0; i < GUSPATSYNTH_CHANNELS; i++) {
		self->channels[i].pitchRatio = 1;
	}
}

void GUSPatSynth_Destroy(GUSPatSynth* self) {
	int i, j, k;

	for(i = 0; i < 128; i++) {
		if(getProgramSet(self, i) == NULL) continue;

		for(j = 0; j < 2; j++) {
			for(k = 0; k < 128; k++) GUSPatSynth_Unload(self, i, k, j);
		}
	}

	if(self->bank.sets != NULL) free(self->bank.sets);

	free(self);
}
