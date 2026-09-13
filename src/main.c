#include <turbosynth/easymidi.h>

#include <config.h>

#include "interface.h"
#include "output.h"
#include "thread.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define PAD 32

Mutex*	mAudio;
Thread* tAudio;
int	nSamples = 0;

EasyMidi*  gEasyMidi;
WaveSynth* gSynth;

int gPlayed = 0;
int gLoop   = 0;

InterfaceMod* iModules[128];
InterfaceMod* iChosen = NULL;
Interface*    iHandle = NULL;

OutputMod* oModules[128];
OutputMod* oChosen = NULL;
Output*	   oHandle = NULL;

void audio_thread(void* arg) {
repeat:;
	while(1) {
		short buffer[BUFSZ * 2];

		Mutex_Lock(mAudio);
		if(EasyMidi_IsFinished(gEasyMidi)) {
			if(gLoop) {
				EasyMidi_Reset(gEasyMidi);
				nSamples = 0;
			} else {
				Mutex_Unlock(mAudio);
				break;
			}
		}
		EasyMidi_RenderShort(gEasyMidi, buffer, BUFSZ);
		Mutex_Unlock(mAudio);

		oChosen->Write(oHandle, buffer);

		while(!oChosen->IsFile && oChosen->BufferedSize(oHandle) >= RATE * 0.5)
#ifdef _WIN32
			Sleep(1);
#else
			usleep(1 * 1000);
#endif

		nSamples += BUFSZ;
	}

	while(!iChosen->Continue && !oChosen->IsFile && oChosen->BufferedSize(oHandle) > 0);

	Mutex_Lock(mAudio);
	gPlayed = 1;
	Mutex_Unlock(mAudio);
}

int startcmp(const char* big, const char* small) {
	int i;

	if(strlen(big) < strlen(small)) return 0;

	for(i = 0; i < strlen(small); i++) {
		if(big[i] != small[i]) return 0;
	}

	return 1;
}

char* padEnd(const char* str, int amount) {
	static char result[257];
	int	    i;

	strcpy(result, str);
	for(i = strlen(str); i < amount; i++) {
		result[i] = ' ';
	}
	result[i] = 0;

	return result;
}

int main(int argc, char** argv) {
	const char* cfg	 = SYSCONFDIR "/pmidi/pmidi.cfg";
	const char* midi = NULL;
	const char* wav	 = NULL;
	int	    i, j;
	const char* outarg = NULL;
	int	    n;

	memset(iModules, 0, sizeof(iModules));
	memset(oModules, 0, sizeof(oModules));

	n	      = 0;
	iModules[n++] = iDumb;

	n	      = 0;
	oModules[n++] = oMiniaudio;
	oModules[n++] = oWave;

	for(i = 1; i < argc; i++) {
		if(startcmp(argv[i], "-C")) {
			cfg = argv[i][2] == 0 ? argv[++i] : (argv[i] + 2);
		} else if(startcmp(argv[i], "-o")) {
			outarg	= argv[i][2] == 0 ? argv[++i] : (argv[i] + 2);
			oChosen = oWave;
		} else if(startcmp(argv[i], "-O")) {
			char* arg = argv[i][2] == 0 ? argv[++i] : (argv[i] + 2);

			if(arg != NULL) {
				for(j = 1; j < sizeof(oModules) / sizeof(oModules[0]); j++) {
					if(strcmp(oModules[j]->Name, arg) == 0 || (strlen(arg) == 1 && oModules[j]->Initial == arg[0])) {
						oChosen = oModules[j];
						break;
					}
				}
			}
		} else if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			printf("Usage: %s [flags] midi\n", argv[0]);
			printf("\n");
			printf("Flags:\n");
			printf("    %s specify config\n", padEnd("-C [config]", PAD));
			printf("    %s set output, also automatically toggles to wave output\n", padEnd("-o [output]", PAD));
			printf("    %s set output module, available:\n", padEnd(" -O [output]", PAD));
			for(j = 0; j < sizeof(oModules) / sizeof(oModules[0]); j++) {
				if(oModules[j] == NULL) continue;
				printf("    %s    `%s' or `%c': %s\n", padEnd("", PAD), oModules[j]->Name, oModules[j]->Initial, oModules[j]->Description);
			}
			printf("    %s set interface module, available:\n", padEnd("-i [interface]", PAD));
			for(j = 0; j < sizeof(iModules) / sizeof(iModules[0]); j++) {
				if(iModules[j] == NULL) continue;
				printf("    %s    `%s' or `%c': %s\n", padEnd("", PAD), iModules[j]->Name, iModules[j]->Initial, iModules[j]->Description);
			}
			printf("    %s enable loop\n", padEnd("-L", PAD));
			return 0;
		} else if(strcmp(argv[i], "-L") == 0) {
			gLoop = 1;
		} else if(argv[i][0] == '-') {
			fprintf(stderr, "invalid flag: %s\n", argv[i]);
		} else {
			midi = argv[i];
		}
	}

	if(cfg == NULL) {
		fprintf(stderr, "specify config file\n");
		return 1;
	}

	if(iChosen == NULL || iHandle == NULL) {
		for(i = 0; i < sizeof(iModules) / sizeof(iModules[0]); i++) {
			if(iModules[i] == NULL) continue;
			if(iChosen != NULL && iModules[i] != iChosen) continue;
			if((iHandle = iModules[i]->New()) != NULL) {
				iChosen = iModules[i];
				break;
			}
		}
	}

	if(iChosen == NULL || iHandle == NULL) {
		fprintf(stderr, "audio device failure\n");
		return 1;
	}

	if(!iChosen->AllowEmpty && midi == NULL) {
		iChosen->Destroy(iHandle);

		fprintf(stderr, "specify midi file\n");
		return 1;
	}

	if((gEasyMidi = EasyMidi_New(cfg, RATE)) == NULL) {
		iChosen->Destroy(iHandle);

		fprintf(stderr, "cannot open gus patches\n");
		return 1;
	}

	gSynth = EasyMidi_GetSynth(gEasyMidi);

	if(midi != NULL && !EasyMidi_Load(gEasyMidi, midi)) {
		iChosen->Destroy(iHandle);
		EasyMidi_Destroy(gEasyMidi);

		fprintf(stderr, "cannot open midi\n");
		return 1;
	}

	if(oChosen == NULL || oHandle == NULL) {
		for(i = 0; i < sizeof(oModules) / sizeof(oModules[0]); i++) {
			if(oModules[i] == NULL) continue;
			if(oChosen != NULL && oModules[i] != oChosen) continue;
			if((oHandle = oModules[i]->New(outarg)) != NULL) {
				oChosen = oModules[i];
				break;
			}
		}
	}

	if(oChosen == NULL || oHandle == NULL) {
		iChosen->Destroy(iHandle);
		EasyMidi_Destroy(gEasyMidi);

		fprintf(stderr, "audio device failure\n");
		return 1;
	}

	mAudio = Mutex_New();
	tAudio = Thread_New(audio_thread, NULL);

	iChosen->Run(iHandle);
	Thread_Wait(tAudio);
	Thread_Destroy(tAudio);
	Mutex_Destroy(mAudio);

	iChosen->Destroy(iHandle);
	oChosen->Destroy(oHandle);
	EasyMidi_Destroy(gEasyMidi);

	return 0;
}
