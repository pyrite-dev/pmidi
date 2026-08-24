#include "filestream.h"

typedef struct StandardFileStream {
	FileStream base;

	unsigned char*	  data;
	FileStreamBigUInt seek;
	FileStreamBigUInt size;
} StandardFileStream;

static int FileStream_ReadImpl(FileStream* self, void* buf, int size) {
	StandardFileStream* sfs = (StandardFileStream*)self;
	int		    sz	= size > (sfs->size - sfs->seek) ? (sfs->size - sfs->seek) : size;

#ifdef DEBUG
	memset(buf, 0, size); /* this is to make valgrind not complain like hell */
#endif

	memcpy(buf, sfs->data + sfs->seek, sz);
	sfs->seek += sz;

	return sz;
}

static void FileStream_SeekImpl(FileStream* self, FileStreamBigUInt pos) {
	((StandardFileStream*)self)->seek = pos;
}

static FileStreamBigUInt FileStream_TellImpl(FileStream* self) {
	return ((StandardFileStream*)self)->seek;
}

static void FileStream_CloseImpl(FileStream* self) {
	free(((StandardFileStream*)self)->data);
}

FileStream* FileStream_New(const char* path) {
	StandardFileStream* self = calloc(1, sizeof(*self));
	FILE*		    fp;

	if((fp = fopen(path, "rb")) == NULL) {
		free(self);

		return NULL;
	}

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
