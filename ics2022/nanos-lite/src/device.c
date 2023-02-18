#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buffer, size_t offset, size_t length)
{
	char *ptr = (char *)buffer;
	while (length > 0) {
    length --;
    putch(*ptr);
    ptr++;
  }
	return 0;
}

size_t events_read(void *buf, size_t offset, size_t len) {
	AM_INPUT_KEYBRD_T key = io_read(AM_INPUT_KEYBRD);
	if (key.keycode == AM_KEY_NONE)
		return 0;
	memset(buf, 0, len);
	if (key.keydown)
		sprintf(buf, "kd %s", keyname[key.keycode]);
	else
		sprintf(buf, "ku %s", keyname[key.keycode]);

	return strlen((char *)buf);
}

size_t dispinfo_read(void *buffer, size_t offset, size_t len) {
	AM_GPU_CONFIG_T screen_cfg = io_read(AM_GPU_CONFIG);

	sprintf(buffer, "[W]: [%d]\n[H]: [%d]", screen_cfg.width, screen_cfg.height);
	return 0;
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
	uint32_t w = io_read(AM_GPU_CONFIG).width;
	uint32_t h = io_read(AM_GPU_CONFIG).height;
	uint32_t x = (offset / 4) % w; 
	uint32_t y = (offset / 4) / w;
	if (offset + len > w * h * 4)
		len = w * h * 4 - offset;
	io_write(AM_GPU_FBDRAW, x, y, (uint32_t *)buf, len / 4, 1, true);
	return len;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
