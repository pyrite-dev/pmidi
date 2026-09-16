#include "interface.h"

#include "output.h"

#define _MILSKO
#include <Mw/Milsko.h>

#include "pmidi.xpm"

#define WhiteKeys 75
#define KeyWidth 6
#define PianoHeight 18
#define ControlHeight 64

static const int whiteBefore[12] = {
    0, 1, 1, 2, 2, 3,
    4, 4, 5, 5, 6, 6};

static int isBlack(int key) {
	int n = key % (5 + 7);

	switch(n) {
	case 1:
	case 3:
	case 6:
	case 8:
	case 10:
		return 1;
	}

	return 0;
}

static int piano_create(MwWidget handle) {
	handle->internal = calloc(128, sizeof(int));
	return 0;
}

static void piano_destroy(MwWidget handle) {
	free(handle->internal);
}

static void piano_draw(MwWidget handle) {
	MwLLColor base	= MwParseColor(handle, MwGetText(handle, MwNbackground));
	MwLLColor white = MwParseColor(handle, "#ffffff");
	MwLLColor black = MwParseColor(handle, "#000000");
	MwLLColor red	= MwParseColor(handle, "#ff0000");
	MwRect	  r;
	double	  getX[128];
	double	  getWidth[128];
	int	  i, j;
	int	  WhiteHeight, BlackHeight;
	int*	  arr = handle->internal;

	r.x	 = 0;
	r.y	 = 0;
	r.width	 = MwGetInteger(handle, MwNwidth);
	r.height = MwGetInteger(handle, MwNheight);
	MwDrawRect(handle, &r, base);

	WhiteHeight = r.height;
	BlackHeight = WhiteHeight / 2;

	for(i = 0; i < 128; i++) {
		double w = (double)r.width / WhiteKeys;
		int    n;
		double x;

		if(isBlack(i)) w = w / 1.25;

		getWidth[i] = w;

		n = (i / 12) * 7 + whiteBefore[i % 12];

		if(isBlack(i)) {
			x = n * getWidth[0] - w / 2;
		} else {
			x = n * w;
		}

		getX[i] = x;
	}

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 128; j++) {
			MwRect r2;

			r2.x	  = getX[j];
			r2.y	  = 0;
			r2.width  = getWidth[j];
			r2.height = isBlack(j) ? BlackHeight : WhiteHeight;

			if(i == 0 && !isBlack(j)) {
				MwDrawRect(handle, &r2, arr[j] ? red : white);
			} else if(i == 1 && !isBlack(j)) {
				MwDrawRectLine(handle, &r2, black);
			} else if(i == 2 && isBlack(j)) {
				MwDrawRect(handle, &r2, arr[j] ? red : black);
			} else if(i == 3 && isBlack(j)) {
				MwDrawRectLine(handle, &r2, black);
			}
		}
	}

	MwDrawRectLine(handle, &r, black);

	MwLLFreeColor(red);
	MwLLFreeColor(black);
	MwLLFreeColor(white);
	MwLLFreeColor(base);
}

MwClassRec MwPianoClassRec = {
    piano_create,  /* create */
    piano_destroy, /* destroy */
    piano_draw,	   /* draw */
    NULL,	   /* click */
    NULL,	   /* parent_resize */
    NULL,	   /* prop_change */
    NULL,	   /* mouse_move */
    NULL,	   /* mouse_up */
    NULL,	   /* mouse_down */
    NULL,	   /* key */
    NULL,	   /* execute */
    NULL,	   /* tick */
    NULL,	   /* resize */
    NULL,	   /* children_update */
    NULL,	   /* children_prop_change */
    NULL,	   /* clipboard */
    NULL,	   /* props_change */
    NULL,
    NULL,
    NULL};
MwClass MwPianoClass = &MwPianoClassRec;

struct Interface {
	MwWidget   window;
	MwWidget   piano[16];
	MwWidget   box[16];
	MwLLPixmap icon;
};

static void window_tick(MwWidget handle, void* user, void* call) {
	Interface* self = MwGetVoid(handle, "Vself");
	int	   i;

	for(i = 0; i < 16; i++) {
		MwForceRender(self->piano[i]);
	}
}

static Interface* New(void) {
	Interface*  self = calloc(1, sizeof(*self));
	MwSizeHints sh;
	int	    i;
	int	    PianoWidth;

	PianoWidth = WhiteKeys * KeyWidth;

	sh.max_width = sh.min_width = PianoWidth * 4 / 3;
	sh.max_height = sh.min_height = PianoHeight * 16 + ControlHeight;

	MwLibraryInit();

	if((self->window = MwVaCreateWidget(MwWindowClass, "window", NULL, MwDEFAULT, MwDEFAULT, sh.max_width, sh.max_height,
					    MwNtitle, "Pyrite MIDI Player",
					    MwNsizeHints, &sh,
					    "Vself", self,
					    NULL)) == NULL) {
		free(self);

		return NULL;
	}

	self->icon = MwLoadXPM(self->window, pmidi);

	MwSetVoid(self->window, MwNiconPixmap, self->icon);

	for(i = 0; i < 16; i++) {
		char t[16];

		sprintf(t, "CH%02d", i + 1);

		self->piano[i] = MwCreateWidget(MwPianoClass, "piano", self->window, sh.max_width - PianoWidth, i * PianoHeight, PianoWidth, PianoHeight);
		self->box[i]   = MwVaCreateWidget(MwBoxClass, "box", self->window, 0, i * PianoHeight, sh.max_width - PianoWidth, PianoHeight,
						  MwNhasBorder, 1,
						  MwNinverted, 1,
						  NULL);

		MwVaCreateWidget(MwLabelClass, "label", self->box[i], 0, 0, 0, 0,
				 MwNtext, t,
				 MwNuseMonospace, 1,
				 MwNalignment, MwALIGNMENT_BEGINNING,
				 MwNfixedSize, MwTextWidth(self->box[i], MwFLBuildFont(MwFLFlagMonospace), t),
				 NULL);
	}

	while(MwPending(self->window)) MwStep(self->window);

	MwAddUserHandler(self->window, MwNtickHandler, window_tick, self);

	return self;
}

static void midi_callback(EasyMidi* self, const MidiEvent* event) {
	Interface* iface = self->user;
	if(event->type == MidiEventNote) {
		int* arr = iface->piano[event->note.channel]->internal;

		arr[event->note.key] = event->note.velocity;
	}
}

static void Run(Interface* self) {
	Mutex_Lock(mAudio);
	gEasyMidi->user = self;
	EasyMidi_SetCallback(gEasyMidi, midi_callback);
	Mutex_Unlock(mAudio);

	MwLoop(self->window);
}

static void Destroy(Interface* self) {
	MwLLDestroyPixmap(self->icon);
	MwDestroyWidget(self->window);
	free(self);
}

static InterfaceMod _interface = {
    New,
    Run,
    Destroy,
    "milsko",
    'm',
    "Milsko iterface",
    1,
    1};
InterfaceMod* iMilsko = &_interface;
