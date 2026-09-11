#include <turbosynth/easymidi.h>

#include <config.h>

#include "output.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static EasyMidi* gEasyMidi;

static void render(short* out, int frames) {
	EasyMidi_RenderShort(gEasyMidi, out, frames);
}

int startcmp(const char* big, const char* small) {
	int i;

	if(strlen(big) < strlen(small)) return 0;

	for(i = 0; i < strlen(small); i++) {
		if(big[i] != small[i]) return 0;
	}

	return 1;
}

int main(int argc, char** argv) {
	const char*   cfg  = SYSCONFDIR "/pmidi/pmidi.cfg";
	const char*   midi = NULL;
	const char*   wav  = NULL;
	int	      i, j;
	output_mod_t* mods[] = {
	    NULL,
	    o_miniaudio,
	    o_wave};
	output_mod_t* omod   = NULL;
	output_t*     output = NULL;
	const char*   outarg = NULL;
	int	      n	     = 0;

	for(i = 1; i < argc; i++) {
		if(startcmp(argv[i], "-C")) {
			cfg = argv[i][2] == 0 ? argv[++i] : (argv[i] + 2);
		} else if(startcmp(argv[i], "-o")) {
			outarg	= argv[i][2] == 0 ? argv[++i] : (argv[i] + 2);
			mods[0] = o_wave;
		} else if(startcmp(argv[i], "-i")) {
			char* arg = argv[i][2] == 0 ? argv[++i] : (argv[i] + 2);

			if(arg != NULL) {
				for(j = 1; j < sizeof(mods) / sizeof(mods[0]); j++) {
					if(strcmp(mods[j]->Name, arg) == 0 || (strlen(arg) == 1 && mods[j]->Initial == arg[0])) {
						omod = mods[j];
						break;
					}
				}
			}
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

	if(omod == NULL || output == NULL) {
		for(i = 0; i < sizeof(mods) / sizeof(mods[0]); i++) {
			if(mods[i] == NULL) continue;
			if(omod != NULL && mods[i] != omod) continue;
			if((output = mods[i]->New(outarg)) != NULL) {
				omod = mods[i];
				break;
			}
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

		while(!omod->IsFile && omod->BufferedSize(output) >= RATE * 0.5)
#ifdef _WIN32
			Sleep(1);
#else
			usleep(1 * 1000);
#endif

		n += BUFSZ;

		printf("%d seconds rendered\r", n / RATE);
		fflush(stdout);
	}

	while(!omod->IsFile && omod->BufferedSize(output) > 0);

	printf("\n");

	omod->Destroy(output);
	EasyMidi_Destroy(gEasyMidi);

	return 0;
}
