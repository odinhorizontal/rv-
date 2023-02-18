#define SDL_malloc malloc
#define SDL_free free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"
#include <stdio.h>

SDL_Surface *IMG_Load_RW(SDL_RWops *src, int freesrc)
{
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

size_t fz(FILE *fp)
{
  fseek(fp, 0, SEEK_END);
  return ftell(fp);
}

SDL_Surface *IMG_Load(const char *filename)
{
  FILE *image = fopen(filename, "r");
  size_t size = fz(image);

  char *img_data = malloc(size);
  fseek(image, 0, SEEK_SET);
  fread(img_data, size, 1, image);

  SDL_Surface *surface = STBIMG_LoadFromMemory(img_data, size);

  free(img_data);
  fclose(image);

  return surface;
}

int IMG_isPNG(SDL_RWops *src)
{
  return 0;
}

SDL_Surface *IMG_LoadJPG_RW(SDL_RWops *src)
{
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError()
{
  return "Navy does not support IMG_GetError()";
}
