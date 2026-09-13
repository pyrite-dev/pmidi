#include "interface.h"

#include "output.h"

struct Interface {
	int dummy;
};

static Interface* New(void) {
	Interface* self = calloc(1, sizeof(*self));

	return self;
}

static void Run(Interface* self) {
	while(1) {
		Mutex_Lock(mAudio);
		if(gPlayed) {
			Mutex_Unlock(mAudio);
			break;
		}
		printf("\r%.2f seconds rendered", (double)nSamples / RATE);
		fflush(stdout);
		Mutex_Unlock(mAudio);
	}

	printf("\n");
}

static void Destroy(Interface* self) {
	free(self);
}

static InterfaceMod interface = {
    New,
    Run,
    Destroy,
    "dumb",
    'd',
    "Dumb terminal interface",
    0,
    0};
InterfaceMod* iDumb = &interface;
