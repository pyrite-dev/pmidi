#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#define RATE 48000
#define BUFSZ (RATE / 100)

typedef struct output output_t;
struct output;

typedef struct output_mod output_mod_t;

struct output_mod {
	output_t* (*New)(const char* output);
	void (*Write)(output_t* self, short* wave);
	int (*BufferedSize)(output_t* self);
	void (*Destroy)(output_t* self);

	const char* Name;
	char	    Initial;
	int	    IsFile;
};

extern output_mod_t* o_miniaudio;
extern output_mod_t* o_wave;

#endif
