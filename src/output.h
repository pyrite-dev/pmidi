#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#define RATE 48000
#define BUFSZ (RATE / 100)

typedef struct Output Output;
struct Output;

typedef struct OutputMod OutputMod;

struct OutputMod {
	Output* (*New)(const char* output);
	void (*Write)(Output* self, short* wave);
	int (*BufferedSize)(Output* self);
	void (*Destroy)(Output* self);

	const char* Name;
	char	    Initial;
	const char* Description;
	int	    IsFile;
};

extern OutputMod* oChosen;
extern Output*	  oHandle;

extern OutputMod* oMiniaudio;
extern OutputMod* oWave;

#endif
