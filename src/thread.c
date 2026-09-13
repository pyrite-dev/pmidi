#include "thread.h"

#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>

struct Thread {
	HANDLE thread;
	void (*callback)(void* user);
	void* user;
};

struct Mutex {
	HANDLE event;
};

static void thread_func(void* arg) {
	Thread* self = arg;

	self->callback(self->user);

	_endthread();
}

Thread* Thread_New(void (*callback)(void* user), void* user) {
	Thread* self = malloc(sizeof(*self));

	self->callback = callback;
	self->user     = user;
	self->thread   = (HANDLE)_beginthread(thread_func, 0, self);

	return self;
}

void Thread_Wait(Thread* self) {
	WaitForSingleObject(self->thread, INFINITE);
}

void Thread_Destroy(Thread* self) {
	CloseHandle(self->thread);
	free(self);
}

Mutex* Mutex_New(void) {
	Mutex* self = malloc(sizeof(*self));

	self->event = CreateEvent(NULL, FALSE, TRUE, NULL);

	return self;
}

void Mutex_Lock(Mutex* self) {
	WaitForSingleObject(self->event, INFINITE);
}

void Mutex_Unlock(Mutex* self) {
	SetEvent(self->event);
}

void Mutex_Destroy(Mutex* self) {
	free(self);
}
#else
struct Thread {
};

struct Mutex {
};
#endif
