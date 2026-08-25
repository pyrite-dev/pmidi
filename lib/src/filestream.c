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

static int FileStream_ReadImpl(FileStream* self, void* buf, int size) {
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

static void FileStream_SeekImpl(FileStream* self, FileStreamBigUInt pos) {
#ifdef LOAD_ALL
	((StandardFileStream*)self)->seek = pos;
#else
	fseek(((StandardFileStream*)self)->fp, pos, SEEK_SET);
#endif
}

static FileStreamBigUInt FileStream_TellImpl(FileStream* self) {
#ifdef LOAD_ALL
	return ((StandardFileStream*)self)->seek;
#else
	return ftell(((StandardFileStream*)self)->fp);
#endif
}

static void FileStream_CloseImpl(FileStream* self) {
#ifdef LOAD_ALL
	free(((StandardFileStream*)self)->data);
#else
	fclose(((StandardFileStream*)self)->fp);
#endif
}

FileStream* FileStream_New(const char* path) {
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

#ifdef DEBUG
	fprintf(stderr, "reading file into memory\n");
#endif

	self->data = malloc(self->size);
	fread(self->data, 1, self->size, fp);

#ifdef DEBUG
	fprintf(stderr, "loaded file\n");
#endif

	fclose(fp);
#else
	self->fp = fp;
#endif

	self->base.New	 = FileStream_New;
	self->base.Read	 = FileStream_ReadImpl;
	self->base.Seek	 = FileStream_SeekImpl;
	self->base.Tell	 = FileStream_TellImpl;
	self->base.Close = FileStream_CloseImpl;

	return (FileStream*)self;
}

void FileStream_Destroy(FileStream* self) {
	FileStream_Close(self);
	free(self);
}
