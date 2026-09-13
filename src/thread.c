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
	CloseHandle(self->event);
	free(self);
}
#else
#include <pthread.h>

struct Thread {
	pthread_t thread;
	void (*callback)(void* user);
	void* user;
};

struct Mutex {
	pthread_mutex_t mutex;
};

static void* thread_func(void* arg) {
	Thread* self = arg;

	self->callback(self->user);

	pthread_exit(NULL);

	return NULL; /* is this needed? */
}

Thread* Thread_New(void (*callback)(void* user), void* user) {
	Thread* self = malloc(sizeof(*self));

	self->callback = callback;
	self->user     = user;

	pthread_create(&self->thread, NULL, thread_func, self);

	return self;
}

void Thread_Wait(Thread* self) {
	void* val;

	pthread_join(self->thread, &val);
}

void Thread_Destroy(Thread* self) {
	free(self);
}

Mutex* Mutex_New(void) {
	Mutex* self = malloc(sizeof(*self));

	pthread_mutex_init(&self->mutex, NULL);

	return self;
}

void Mutex_Lock(Mutex* self) {
	pthread_mutex_lock(&self->mutex);
}

void Mutex_Unlock(Mutex* self) {
	pthread_mutex_unlock(&self->mutex);
}

void Mutex_Destroy(Mutex* self) {
	pthread_mutex_destroy(&self->mutex);
	free(self);
}
#endif
