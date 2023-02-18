#include <fs.h>
#include <device.h>

typedef size_t (*ReadFn)(void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn)(const void *buf, size_t offset, size_t len);

typedef struct
{
	char *name;
	size_t size;
	size_t disk_offset;
	ReadFn read;
	WriteFn write;
	size_t prooff;
} Finfo;

enum
{
	FD_STDIN,
	FD_STDOUT,
	FD_STDERR,
	FD_EVENT,
	FD_DISPINFO,
	FD_FB
};

size_t invalid_read(void *buf, size_t offset, size_t len)
{
	panic("should not reach here");
	return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len)
{
	panic("should not reach here");
	return 0;
}

/* This is the information about all files in disk. */
static Finfo thefiles[] __attribute__((used)) = {
	[FD_STDIN] = {"stdin", 0, 0, invalid_read, invalid_write},
	[FD_STDOUT] = {"stdout", 0, 0, invalid_read, invalid_write},
	[FD_STDERR] = {"stderr", 0, 0, invalid_read, invalid_write},
	[FD_EVENT] = {"/dev/events", 0, 0, events_read, invalid_write},
	[FD_DISPINFO] = {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
	[FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write},
#include "files.h"
};

void init_fs()
{
	uint32_t w = io_read(AM_GPU_CONFIG).width;
	uint32_t h = io_read(AM_GPU_CONFIG).height;
	thefiles[FD_FB].size = w * h * 4;
}

int fs_open(const char *path, int flags, int mode)
{
	int k = 0;
	for (int i = 0; i < (sizeof(thefiles) / sizeof(Finfo)); i++, k++)
	{
		if (strcmp(path, thefiles[i].name) == 0)
		{
			Log("fs_open file name %s", thefiles[i].name);
			thefiles[i].prooff = 0;
			return i;
		}
	}
	assert(k != sizeof(thefiles) / sizeof(Finfo));
	return -1;
}

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

size_t fs_read(int fd, void *buf, size_t length)
{
	size_t ret = -1;
	size_t need_to_read = 0;
	size_t fsize = thefiles[fd].size;
	size_t prooff = thefiles[fd].prooff;

	if ( length > fsize - prooff ) {
		need_to_read = fsize - prooff;
	} else {
		need_to_read = length;
	}

	if ( need_to_read <= 0 ) {
		need_to_read = 0;
	}


	if (thefiles[fd].read == NULL)
	{
		ret = ramdisk_read(buf, thefiles[fd].disk_offset + thefiles[fd].prooff, need_to_read);
		thefiles[fd].prooff += need_to_read;
	}
	else
	{
		return thefiles[fd].read(buf, 0, length);
	}

	return ret;
}

size_t fs_write(int fd, const void *buf, size_t len)
{
	size_t ret = -1;
	size_t wlength = 0;
	size_t filesz = thefiles[fd].size;
	size_t prooff = thefiles[fd].prooff;

	if ( len > filesz - prooff ) {
		wlength = filesz - prooff;
	} else {
		wlength = len;
	}

	if ( wlength <= 0 ) wlength = 0;

	size_t offset = thefiles[fd].disk_offset + thefiles[fd].prooff;
	if (thefiles[fd].write == NULL)
	{
		ret = ramdisk_write(buf, offset, wlength);
		thefiles[fd].prooff += wlength;
	}
	else
	{
		return thefiles[fd].write(buf, offset, len);
	}

	return ret;
}

size_t fs_lseek(int fd, size_t offset, int whence)
{
	if (whence == SEEK_SET ) {
		thefiles[fd].prooff = offset;
	} else if (whence == SEEK_CUR) {
		thefiles[fd].prooff += offset;
	} else if ( whence == SEEK_END ) {
		thefiles[fd].prooff = thefiles[fd].size + offset;
	} else {
		panic("Unknown whence %d", whence);
	}
	
	return thefiles[fd].prooff;
}

int fs_close(int fd)
{
	thefiles[fd].prooff = 0;
	return 0;
}
