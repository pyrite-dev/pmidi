#ifndef __THREAD_H__
#define __THREAD_H__

typedef struct Thread Thread;
struct Thread;

typedef struct Mutex Mutex;
struct Mutex;

Thread* Thread_New(void (*callback)(void* user), void* user);
void	Thread_Wait(Thread* self);
void	Thread_Destroy(Thread* self);

Mutex* Mutex_New(void);
void   Mutex_Lock(Mutex* self);
void   Mutex_Unlock(Mutex* self);
void   Mutex_Destroy(Mutex* self);

#endif
