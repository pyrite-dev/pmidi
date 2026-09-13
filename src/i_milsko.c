#include "interface.h"

#include "output.h"

#include <Mw/Milsko.h>

#include "pmidi.xpm"

struct Interface {
	MwWidget   window;
	MwLLPixmap icon;
};

static void window_tick(MwWidget handle, void* user, void* call) {
}

static Interface* New(void) {
	Interface*  self = calloc(1, sizeof(*self));
	MwSizeHints sh;

	sh.max_width = sh.min_width = 400;
	sh.max_height = sh.min_height = sh.max_width / 4;

	MwLibraryInit();

	if((self->window = MwVaCreateWidget(MwWindowClass, "window", NULL, MwDEFAULT, MwDEFAULT, sh.max_width, sh.max_height,
					    MwNtitle, "Pyrite MIDI Player",
					    MwNsizeHints, &sh,
					    NULL)) == NULL) {
		free(self);

		return NULL;
	}

	self->icon = MwLoadXPM(self->window, pmidi);

	MwSetVoid(self->window, MwNiconPixmap, self->icon);

	while(MwStep(self->window));

	MwAddUserHandler(self->window, MwNtickHandler, window_tick, self);

	return self;
}

static void Run(Interface* self) {
	MwLoop(self->window);
}

static void Destroy(Interface* self) {
	MwLLDestroyPixmap(self->icon);
	MwDestroyWidget(self->window);
	free(self);
}

static InterfaceMod interface = {
    New,
    Run,
    Destroy,
    "milsko",
    'm',
    "Milsko iterface",
    1,
    1};
InterfaceMod* iMilsko = &interface;
