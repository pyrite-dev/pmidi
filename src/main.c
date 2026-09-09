#include <turbosynth/easymidi.h>

#include <config.h>

#include "output.h"

static EasyMidi* gEasyMidi;

static void render(short* out, int frames) {
	EasyMidi_RenderShort(gEasyMidi, out, frames);
}

int main(int argc, char** argv) {
	const char*   cfg  = SYSCONFDIR "/pmidi/pmidi.cfg";
	const char*   midi = NULL;
	const char*   wav  = NULL;
	int	      i;
	output_mod_t* mods[] = {
	    o_miniaudio,
	    o_wave,
	    NULL};
	output_mod_t* omod   = NULL;
	output_t*     output = NULL;
	const char*   outarg = NULL;
	int	      n	     = 0;

	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-C") == 0) {
			cfg = argv[i][2] == 0 ? argv[++i] : argv[i];
		} else if(strcmp(argv[i], "-o") == 0) {
			outarg = argv[i][2] == 0 ? argv[++i] : argv[i];
		} else if(argv[i][0] == '-') {
		} else {
			midi = argv[i];
		}
	}

	if(cfg == NULL) {
		fprintf(stderr, "specify config file\n");
		return 1;
	}

	if(midi == NULL) {
		fprintf(stderr, "specify midi file\n");
		return 1;
	}

	if((gEasyMidi = EasyMidi_New(cfg, RATE)) == NULL) {
		fprintf(stderr, "cannot open gus patches\n");
		return 1;
	}

	if(!EasyMidi_Load(gEasyMidi, midi)) {
		EasyMidi_Destroy(gEasyMidi);

		fprintf(stderr, "cannot open midi\n");
		return 1;
	}

	for(i = 0; i < sizeof(mods) / sizeof(mods[0]); i++) {
		if((output = mods[i]->New(outarg)) != NULL) {
			omod = mods[i];
			break;
		}
	}

	if(omod == NULL || output == NULL) {
		EasyMidi_Destroy(gEasyMidi);

		fprintf(stderr, "audio device failure\n");
		return 1;
	}

	while(1) {
		int   i;
		short buffer[BUFSZ * 2];

		if(EasyMidi_IsFinished(gEasyMidi)) break;

		render(buffer, BUFSZ);
		omod->Write(output, buffer);

		n += BUFSZ;

		printf("%d seconds rendered\r", n / RATE);
		fflush(stdout);
	}

	printf("\n");

	omod->Destroy(output);
	EasyMidi_Destroy(gEasyMidi);

	return 0;
}
