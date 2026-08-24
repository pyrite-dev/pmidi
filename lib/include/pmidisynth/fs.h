#ifndef __PMIDISYNTH_FILESTREAM_H__
#define __PMIDISYNTH_FILESTREAM_H__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <stdint.h>
typedef int64_t	 FileStreamBigInt;
typedef uint64_t FileStreamBigUInt;
#elif defined(__WATCOMC__)
typedef __int64		 FileStreamBigInt;
typedef unsigned __int64 FileStreamBigUInt;
#else
typedef long		  FileStreamBigInt;  /* :( */
typedef FileStreamBigUInt FileStreamBigUInt; /* :( */
#endif

typedef struct FileStream FileStream;

struct FileStream {
	int (*Read)(FileStream* self, void* buf, int size);
	void (*Seek)(FileStream* self, FileStreamBigUInt pos);
	FileStreamBigUInt (*Tell)(FileStream* self);
	void (*Close)(FileStream* self);
};

FileStream* FileStream_New(const char* path);
#define FileStream_Read(self, buf, size) (self)->Read((self), (buf), (size))
#define FileStream_Seek(self, pos) (self)->Seek((self), (pos))
#define FileStream_Tell(self) (self)->Tell((self))
#define FileStream_Close(self) (self)->Close((self))
void FileStream_Destroy(FileStream* self);

#endif
