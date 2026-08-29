#ifndef __TURBOSYNTH_FILESTREAM_H__
#define __TURBOSYNTH_FILESTREAM_H__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || (defined(__cplusplus) && __cplusplus >= 199901L)
#include <stdint.h>
typedef int64_t	 FileStreamBigInt;
typedef uint64_t FileStreamBigUInt;
#elif defined(__WATCOMC__)
typedef __int64		 FileStreamBigInt;
typedef unsigned __int64 FileStreamBigUInt;
#else
typedef long	      FileStreamBigInt;	 /* :( */
typedef unsigned long FileStreamBigUInt; /* :( */
#endif

typedef struct FileStream FileStream;

struct FileStream {
	FileStream* (*New)(const char* path, void* arg);
	int (*Read)(FileStream* self, void* buf, int size);
	void (*Seek)(FileStream* self, FileStreamBigUInt pos);
	FileStreamBigUInt (*Tell)(FileStream* self);
	void (*Close)(FileStream* self);

	char* path;
	void* newArg;
};

#ifdef __cplusplus
extern "C" {
#endif

FileStream* FileStream_New(const char* path, void* arg);
#define FileStream_Read(self, buf, size) (self)->Read((self), (buf), (size))
#define FileStream_Seek(self, pos) (self)->Seek((self), (pos))
#define FileStream_Tell(self) (self)->Tell((self))
#define FileStream_Close(self) (self)->Close((self))
void FileStream_Destroy(FileStream* self);

#ifdef __cplusplus
}
#endif

#endif
