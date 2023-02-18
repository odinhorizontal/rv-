#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static char *process_p(char *p, va_list *ap);
static char *process_ld(char *p, va_list *ap);
static char *process_x(char *p, va_list *ap);
static char* process_s(char *p, va_list* ap);
static char *process_d(char *p, va_list *ap);


static int process_param(char *buffer, const char *fmtstr, va_list ap)
{
	char c, *ptr = (char *)buffer;
	
	while (*fmtstr)
	{
		if (*fmtstr == '%')
		{
			fmtstr++;
			if (*fmtstr == 'p') {
				fmtstr++;
				ptr = process_p(ptr, &ap);
			} else if ( *fmtstr == 'x') {
				fmtstr++;
				ptr = process_x(ptr, &ap);
			} else if ( *fmtstr == 'd') {
				fmtstr++;
				ptr = process_d(ptr, &ap);
			} else if ( *fmtstr == 'l') {
				fmtstr++;
				if (*fmtstr++ == 'd') ptr = process_ld(ptr, &ap);
			} else if ( *fmtstr == 'c') {
				fmtstr++;
				c = (char)va_arg(ap, int);
				*ptr++ = c;
			} else if ( *fmtstr == 's') { 
				fmtstr++;
				ptr = process_s(ptr, &ap);
			}
		}
		else {
			*ptr = *fmtstr;
			ptr++;
			fmtstr++;
		}
	}
	*ptr++ = '\0';
	return 0;
}

int printf(const char *fmt, ...)
{
	char buffer[2048];
	va_list ap;
	va_start(ap, fmt);
	process_param(buffer, fmt, ap);
	va_end(ap);
	putstr(buffer);
	return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap)
{
	panic("Not implemented");
}

int sprintf(char *buffer, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	process_param(buffer, fmt, ap);
	va_end(ap);
	return 0;
}

int snprintf(char *out, size_t n, const char *fmt, ...)
{
	panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap)
{
	panic("Not implemented");
}

static char *process_p(char *p, va_list *ap)
{
	unsigned int *pd, idx = 0, buffer[200];
	pd = va_arg(*ap, unsigned int*);
	unsigned int d = (unsigned int)pd;
	*p = '0';
	p++;
	*p = 'x';
	p++;
	if (d == 0)
	{
		*p++ = '0';
	}
	else
	{
		idx = 0;
		while (d)
		{
			buffer[idx] = d % 16;
			d /= 16;
			idx++;
		}
		while (idx--) {
			if (buffer[idx] < 10)
				*p = buffer[idx] + '0';
			else
				*p = buffer[idx] + 'a' - 10;
			
			p++;
		}
	}
	return p;
}

static char *process_ld(char *p, va_list *ap)
{
	int idx = 0, buffer[200];
	uint64_t ld;
	ld = va_arg(*ap, __uint64_t);
	while (ld)
	{
		buffer[idx] = ld % 10 + '0';
		ld /= 10;
		idx++;
	}
	while (idx--) {
		*p = buffer[idx];
		p++;
	}
	return p;
}

static char *process_x(char *p, va_list *ap)
{
	unsigned int d, buffer[200];
	unsigned idx = 0;
	d = va_arg(*ap, unsigned int);
	if (d == 0)
	{
		*p = '0';
		p++;
	}
	else
	{
		idx = 0;
		while (d)
		{
			buffer[idx] = d % 16;
			d /= 16;
			idx++;
		}
		while (idx--) {
			if (buffer[idx] < 10)
				*p = buffer[idx] + '0';
			else
				*p= buffer[idx] + 'a' - 10;
			p++;
		}				
	}
	return p;
}

static char* process_s(char *p, va_list* ap)
{
	char *s;
	s = va_arg(*ap, char *);
	while (*s) {
		*p = *s;
		p++;
		s++;
	}
	return p;
}

static char *process_d(char *p, va_list *ap)
{
	int d, idx = 0, buffer[200];
	d = va_arg(*ap, int);
	if (d == 0)
	{
		*p = '0';
		p++;
	}
	else if (d > 0)
	{
		idx = 0;
		while (d)
		{
			buffer[idx] = d % 10 + '0';
			d /= 10;
			idx++;
		}
		while (idx--) {
			*p = buffer[idx];
			p++;
		}
	}
	else
	{
		*p = '-';
		p++;
		d = -d;
		idx = 0;
		while (d)
		{
			buffer[idx] = d % 10 + '0';
			d /= 10;
			idx++;
		}
		while (idx--) {
			*p = buffer[idx];
			p++;
		}
	}
	return p;
}

#endif
