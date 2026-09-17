#include "interface.h"

#include "output.h"

#define _MILSKO
#include <Mw/Milsko.h>

#include "pmidi.xpm"

#define WhiteKeys 75
#define KeyWidth 6
#define PianoHeight 18
#define ControlHeight 90
#define ControlButtonHeight 25

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
	MwWidget window;
	MwWidget menu;
	MwWidget piano[16];
	MwWidget box[16];
	MwWidget prgbnk[16];
	MwWidget lcd0;

	MwLLPixmap icon;

	int update_piano[16];

	int program[16];
	int bank[16];
	int update_prgbnk[16];
};

static void window_tick(MwWidget handle, void* user, void* call) {
	Interface* self = user;
	int	   i;

	for(i = 0; i < 16; i++) {
		if(self->update_prgbnk[i]) {
			char buf[8];

			sprintf(buf, "%3d %3d", self->program[i] + 1, self->bank[i]);

			MwVaApply(self->prgbnk[i],
				  MwNtext, buf,
				  NULL);

			self->update_prgbnk[i] = 0;
		}

		if(self->update_piano[i]) {
			MwForceRender(self->piano[i]);
			self->update_piano[i] = 0;
		}
	}
}

static void reset(Interface* self) {
	int i;

	gPaused = 0;
	Mutex_Lock(mAudio);
	EasyMidi_Reset(gEasyMidi);

	for(i = 0; i < 16; i++) {
		memset(self->program, 0, sizeof(self->program));
		memset(self->bank, 0, sizeof(self->bank));
		self->update_prgbnk[i] = 1;
		memset(self->piano[i]->internal, 0, sizeof(int) * 128);
		self->update_piano[i] = 1;
	}
	Mutex_Unlock(mAudio);
}

static void button_activate(MwWidget handle, void* user, void* call) {
	Interface* self = user;

	if(strcmp(handle->name, "pause") == 0) {
		gPaused = !gPaused;
	} else if(strcmp(handle->name, "stop") == 0) {
		reset(self);
	} else if(strcmp(handle->name, "play") == 0) {
		int do_reset = 0;

		Mutex_Lock(mAudio);
		if(!gLoop && EasyMidi_IsFinished(gEasyMidi)) do_reset = 1;
		Mutex_Unlock(mAudio);

		if(do_reset) reset(self);

		gPaused = 0;
	}
}

static void button_draw(MwWidget handle) {
	MwLLColor color = MwParseColor(handle, MwGetText(handle, MwNforeground));
	int	  aw	= MwGetInteger(handle, MwNwidth);
	int	  ah	= MwGetInteger(handle, MwNheight);
	int	  w	= aw < ah ? aw : ah;
	int	  bw	= MwDefaultBorderWidth(handle);
	int	  l	= w - bw * 4;

	if(strcmp(handle->name, "stop") == 0) {
		MwRect r;

		r.width	 = l;
		r.height = l;
		r.x	 = (aw - r.width) / 2;
		r.y	 = (ah - r.height) / 2;

		MwDrawRect(handle, &r, color);
	} else if(strcmp(handle->name, "pause") == 0) {
		MwRect r;

		r.width	 = l / 3;
		r.height = l;
		r.x	 = (aw - r.width * 3) / 2;
		r.y	 = (ah - r.height) / 2;

		MwDrawRect(handle, &r, color);

		r.x += r.width * 2;
		MwDrawRect(handle, &r, color);
	} else if(strcmp(handle->name, "play") == 0) {
		MwPoint p[3];

		p[0].x = (aw - l) / 2;
		p[0].y = (ah - l) / 2;
		p[1].x = (aw - l) / 2;
		p[1].y = (ah - l) / 2 + l;
		p[2].x = (aw - l) / 2 + l;
		p[2].y = ah / 2;

		MwLLPolygon(handle->lowlevel, p, 3, color);
	}

	MwLLFreeColor(color);
}

#define LABELPROPS MwNuseMonospace, 1, \
		   MwNalignment, MwALIGNMENT_BEGINNING

static Interface* New(void) {
	Interface*  self = calloc(1, sizeof(*self));
	MwSizeHints sh;
	int	    i;
	int	    PianoWidth;
	MwWidget    cbox, cbox2, cbox3, cbox4, cbox5;
	MwWidget    btn;
	int	    y;

	for(i = 0; i < 16; i++) {
		self->update_prgbnk[i] = 1;
	}

	PianoWidth = WhiteKeys * KeyWidth;

	sh.max_width = sh.min_width = PianoWidth * 4 / 3;
	sh.max_height = sh.min_height = PianoHeight * 17 + ControlHeight;

	MwLibraryInit();

	if((self->window = MwVaCreateWidget(MwWindowClass, "window", NULL, MwDEFAULT, MwDEFAULT, sh.max_width, sh.max_height,
					    MwNtitle, "Pyrite MIDI Player",
					    NULL)) == NULL) {
		free(self);

		return NULL;
	}

	self->menu = MwCreateWidget(MwMenuClass, "menu", self->window, 0, 0, 0, 0);
	MwMenuAdd(self->menu, NULL, "File");

	sh.min_height += MwGetInteger(self->menu, MwNheight);
	sh.max_height += MwGetInteger(self->menu, MwNheight);

	MwVaApply(self->window,
		  MwNwidth, sh.max_width,
		  MwNheight, sh.max_height,
		  MwNsizeHints, &sh,
		  NULL);

	self->icon = MwLoadXPM(self->window, pmidi_xpm);

	MwSetVoid(self->window, MwNiconPixmap, self->icon);

	y = MwGetInteger(self->menu, MwNheight);

	for(i = 0; i < 17; i++, y += PianoHeight) {
		char t[16];
		int  l3sz = MwTextWidth(self->window, MwFLBuildFont(MwFLFlagMonospace), "CH#");
		int  spsz = MwTextWidth(self->window, MwFLBuildFont(MwFLFlagMonospace), ".");

		if(i == 0) {
			MwWidget bl = MwVaCreateWidget(MwBoxClass, "box", self->window, 0, y, sh.max_width, PianoHeight,
						       MwNhasBorder, 1,
						       MwNinverted, 1,
						       NULL);

			MwVaCreateWidget(MwLabelClass, "label", bl, 0, 0, 0, 0,
					 MwNtext, "CH#",
					 LABELPROPS,
					 MwNfixedSize, l3sz,
					 NULL);

			MwVaCreateWidget(MwLabelClass, "label", bl, 0, 0, 0, 0,
					 MwNtext, " PRG BNK",
					 LABELPROPS,
					 MwNfixedSize, spsz + l3sz + spsz + l3sz,
					 NULL);

			continue;
		}

		sprintf(t, "%3d", i);

		self->piano[i - 1] = MwCreateWidget(MwPianoClass, "piano", self->window, sh.max_width - PianoWidth, y, PianoWidth, PianoHeight);
		self->box[i - 1]   = MwVaCreateWidget(MwBoxClass, "box", self->window, 0, y, sh.max_width - PianoWidth, PianoHeight,
						      MwNhasBorder, 1,
						      MwNinverted, 1,
						      NULL);

		MwVaCreateWidget(MwLabelClass, "label", self->box[i - 1], 0, 0, 0, 0,
				 MwNtext, t,
				 LABELPROPS,
				 MwNfixedSize, l3sz,
				 NULL);

		MwVaCreateWidget(MwLabelClass, "label", self->box[i - 1], 0, 0, 0, 0,
				 LABELPROPS,
				 MwNfixedSize, spsz,
				 NULL);

		self->prgbnk[i - 1] = MwVaCreateWidget(MwLabelClass, "label", self->box[i - 1], 0, 0, 0, 0,
						       LABELPROPS,
						       MwNfixedSize, l3sz + spsz + l3sz,
						       NULL);
	}

	cbox = MwCreateWidget(MwBoxClass, "controlbox", self->window, 0, y, sh.max_width, ControlHeight);

	cbox2 = MwVaCreateWidget(MwBoxClass, "controlbox2", cbox, 0, 0, 0, 0,
				 MwNfixedSize, sh.max_width - PianoWidth,
				 MwNhasBorder, 1,
				 MwNinverted, 1,
				 NULL);

	cbox3 = MwVaCreateWidget(MwBoxClass, "controlbox3", cbox, 0, 0, 0, 0,
				 MwNorientation, MwVERTICAL,
				 NULL);

	cbox4 = MwVaCreateWidget(MwBoxClass, "controlbox4", cbox3, 0, 0, 0, 0,
				 MwNhasBorder, 1,
				 MwNinverted, 1,
				 NULL);

	cbox5 = MwVaCreateWidget(MwBoxClass, "controlbox5", cbox3, 0, 0, 0, 0,
				 MwNfixedSize, ControlButtonHeight,
				 MwNhasBorder, 1,
				 MwNinverted, 1,
				 NULL);

	btn = MwVaCreateWidget(MwButtonClass, "stop", cbox5, 0, 0, 0, 0,
			       MwNfixedSize, ControlButtonHeight,
			       NULL);
	MwAddUserHandler(btn, MwNactivateHandler, button_activate, self);
	btn->draw_inject = button_draw;

	btn = MwVaCreateWidget(MwButtonClass, "pause", cbox5, 0, 0, 0, 0,
			       MwNfixedSize, ControlButtonHeight,
			       NULL);
	MwAddUserHandler(btn, MwNactivateHandler, button_activate, self);
	btn->draw_inject = button_draw;

	btn = MwVaCreateWidget(MwButtonClass, "play", cbox5, 0, 0, 0, 0,
			       MwNfixedSize, ControlButtonHeight,
			       NULL);
	MwAddUserHandler(btn, MwNactivateHandler, button_activate, self);
	btn->draw_inject = button_draw;

	while(MwPending(self->window)) MwStep(self->window);

	MwAddUserHandler(self->window, MwNtickHandler, window_tick, self);

	return self;
}

static void midi_callback(EasyMidi* self, const MidiEvent* event) {
	Interface* iface = self->user;
	if(event->type == MidiEventNote) {
		int* arr = iface->piano[event->note.channel]->internal;

		arr[event->note.key]			 = event->note.velocity;
		iface->update_piano[event->note.channel] = 1;
	} else if(event->type == MidiEventProgramChange) {
		WaveSynth* synth = EasyMidi_GetSynth(self);

		iface->program[event->programChange.channel]	   = synth->channels[event->programChange.channel].program;
		iface->bank[event->programChange.channel]	   = synth->channels[event->programChange.channel].bank;
		iface->update_prgbnk[event->programChange.channel] = 1;
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
