#include <turbosynth/fs.h>

// #define LOAD_ALL

typedef struct StandardFileStream {
	FileStream base;

#ifdef LOAD_ALL
	unsigned char*	  data;
	FileStreamBigUInt seek;
	FileStreamBigUInt size;
#else
	FILE* fp;
#endif
} StandardFileStream;

static int StandardFileStream_ReadImpl(FileStream* self, void* buf, int size) {
	StandardFileStream* sfs = (StandardFileStream*)self;
#ifdef LOAD_ALL
	int sz = size > (sfs->size - sfs->seek) ? (sfs->size - sfs->seek) : size;

#ifdef DEBUG
	memset(buf, 0, size); /* this is to make valgrind not complain like hell */
#endif

	memcpy(buf, sfs->data + sfs->seek, sz);
	sfs->seek += sz;
#else
	int sz;

#ifdef DEBUG
	memset(buf, 0, size); /* this is to make valgrind not complain like hell */
#endif

	sz = fread(buf, 1, size, sfs->fp);
#endif

	return sz;
}

static void StandardFileStream_SeekImpl(FileStream* self, FileStreamBigUInt pos) {
#ifdef LOAD_ALL
	((StandardFileStream*)self)->seek = pos;
#else
	fseek(((StandardFileStream*)self)->fp, pos, SEEK_SET);
#endif
}

static FileStreamBigUInt StandardFileStream_TellImpl(FileStream* self) {
#ifdef LOAD_ALL
	return ((StandardFileStream*)self)->seek;
#else
	return ftell(((StandardFileStream*)self)->fp);
#endif
}

static void StandardFileStream_CloseImpl(FileStream* self) {
	StandardFileStream* sfs = (StandardFileStream*)self;

	free(self->path);
#ifdef LOAD_ALL
	free(sfs->data);
#else
	fclose(sfs->fp);
#endif
}

FileStream* FileStream_New(const char* path, void* arg) {
	StandardFileStream* self = calloc(1, sizeof(*self));
	FILE*		    fp;

	if((fp = fopen(path, "rb")) == NULL) {
		free(self);

		return NULL;
	}

#ifdef LOAD_ALL
	self->seek = 0;

	fseek(fp, 0, SEEK_END);
	self->size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	self->data = malloc(self->size);
	fread(self->data, 1, self->size, fp);

	fclose(fp);
#else
	self->fp = fp;
#endif

	self->base.New	  = FileStream_New;
	self->base.Read	  = StandardFileStream_ReadImpl;
	self->base.Seek	  = StandardFileStream_SeekImpl;
	self->base.Tell	  = StandardFileStream_TellImpl;
	self->base.Close  = StandardFileStream_CloseImpl;
	self->base.newArg = arg;
	self->base.path	  = malloc(strlen(path) + 1);
	strcpy(self->base.path, path);

	return (FileStream*)self;
}

void FileStream_Destroy(FileStream* self) {
	FileStream_Close(self);
	free(self);
}
