#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s)
{
  size_t res = 0;
  while (*s != '\0')
  {
    res++;
    s++;
  }
  return res;
}

char *strcpy(char *dst, const char *src)
{
  size_t i = 0;
  while (src[i] != '\0')
  {
    dst[i] = src[i];
    i++;
  }

  dst[i] = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n)
{
  size_t i;
  for (i = 0; i < n; i++)
  {
    if (src[i] == '\0')
    {
      break;
    }
    dst[i] = src[i];
  }

  for (; i < n; i++)
    dst[i] = '\0';

  return dst;
}

char *strcat(char *dst, const char *src)
{
  size_t len = strlen(dst);
  size_t i;

  for (i = 0; src[i] != '\0'; i++)
    dst[len + i] = src[i];

  dst[len + i] = '\0';
  return dst;
}

int strcmp(const char *str1, const char *str2)
{
  while (*str1 == *str2)
  {
    assert((str1 != NULL) && (str2 != NULL));
    if (*str1 == '\0')
      return 0;
    str1++;
    str2++;
  }
  return *str1 - *str2;
}

int strncmp(const char *s1, const char *s2, size_t len)
{
  assert(s1 != NULL && s2 != NULL);

  while (len--)
  {
    if (*s1 == 0 || *s1 != *s2)
      return *s1 - *s2;

    s1++;
    s2++;
  }
  return 0;
}

void *memset(void *s, int c, size_t n)
{
  for (size_t i = 0; i < n; ++i)
  {
    *((char *)s + i) = (char)c;
  }
  return s;
}

void *memcpy(void *out, const void *in, size_t n)
{
  char *chout = (char *)out;
  char *chin = (char *)in;
  while (n > 0)
  {
    *chout = *chin;
    chout++;
    chin++;
    n--;
  }

  return out;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
  int ret = 0;
  char *p1 = (char *)s1;
  char *p2 = (char *)s2;

  while (n > 0)
  {
    ret = *p1 - *p2;
    if (!ret)
    {
      break;
    }
    p1++;
    p2++;
  }

  return ret;
}

void *memmove(void *dst, const void *src, size_t n)
{
  char *out = (char *)dst;
  char *in = (char *)src;
  if (src >= dst)
  {
    while (n > 0)
    {
      *out++ = *in++;
      n--;
    }
  }
  else
  {
    out += n;
    in += n;
    while (n > 0)
    {
      *out-- = *in--;
      n--;
    }
  }
  return dst;
}

#endif
