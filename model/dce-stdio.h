#ifndef SIMU_STDIO_H
#define SIMU_STDIO_H

#include <stdio.h>
#include <stdio_ext.h>
#include <unistd.h>

#include "dce-guard.h"

DCE(FILE *, fopen , const char *path, const char *mode);
DCE(FILE *, fopen64 , const char *path, const char *mode);
DCE(FILE *, fdopen , int fildes, const char *mode);
DCE(FILE *, freopen , const char *path, const char *mode, FILE *stream);
DCE(int, fclose , FILE *fp);
DCE(int, fclose_unconditional , FILE *fp);
DCE(int, fclose_onexec , FILE *file);
DCE(int, fcloseall);
DCE(int, fflush , FILE *stream);
DCE(void, clearerr , FILE *stream);
DCE(int, feof , FILE *stream);
DCE(int, ferror , FILE *stream);
DCE(int, fileno , FILE *stream);
DCE(int, fseek , FILE *stream, long offset, int whence);
DCE(int, fseeko , FILE *stream, off_t offset, int whence);
DCE(long, ftell , FILE *stream);
DCE(off_t, ftello , FILE *stream);
DCE(int, fgetpos , FILE *stream, fpos_t *pos);
DCE(int, fsetpos , FILE *stream, const fpos_t *pos);
DCE(void, rewind , FILE *stream);
DCE(int, setvbuf, FILE *stream, char *buf, int mode, size_t size);
DCE(void, setbuf , FILE *stream, char *buf);
DCE(void, setbuffer , FILE *stream, char *buf, size_t size);
DCE(void, setlinebuf , FILE *stream);

DCE(size_t, fread, void *ptr, size_t size, size_t nmemb, FILE *stream);
DCE(size_t, fwrite , const void *ptr, size_t size, size_t nmemb, FILE *stream);
DCE(int, fputc , int c, FILE *stream);
DCE(int, fputs , const char *s, FILE *stream);
DCE(int, fgetc , FILE *stream);
DCE(char*, fgets , char *s, int size, FILE *stream);
DCE(int, ungetc , int c, FILE *stream);
DCE(int, remove , const char *pathname);

DCE(int, printf , const char *format, ...);
DCE(int, getchar);
DCE(int, _IO_getc , FILE *stream);
DCE(int, putchar , int __c);
DCE(int, _IO_putc , int __c, FILE *__stream);
DCE(int, puts , const char *__s);
DCE(void, perror , const char *s);

DCE(int, __printf_chk , int __flag, __const char *__restrict __format, ...);
DCE(int, __vfprintf_chk , FILE *__restrict __stream, int __flag, __const char *__restrict __format, _G_va_list __ap);
DCE(int, __fprintf_chk , FILE *__restrict __stream, int __flag, __const char *__restrict __format, ...);
DCE(int, __snprintf_chk , char *__restrict __s, size_t __n, int __flag, size_t __slen, __const char *__restrict __format, ...);
DCE(void, __fpurge , FILE *stream);
DCE(size_t, __fpending , FILE *stream);
DCE(int, asprintf , char **strp, const char *fmt, ...);
DCE(int, vasprintf , char **strp, const char *fmt, va_list ap);
DCE(int, vsnprintf , char *s, size_t si, const char *f, va_list ap);
DCE(int, __vsnprintf_chk , char *__restrict __s, size_t __n, int __flag, size_t __slen, __const char *__restrict __format, _G_va_list __ap);



//DCE_WITH_ALIAS2 (clearerr,clearerr_unlocked) REGRESSION
NATIVE (fprintf, FILE *stream, const char *format, ...)
NATIVE (sprintf, char *str, const char *format, ...)
NATIVE (dprintf, int fd, const char *format, ...)
NATIVE (vdprintf, int fd, const char *format, va_list ap)
DCE_ALIAS (fgetc,fgetc_unlocked)
NATIVE (getc, FILE *stream)
NATIVE (getc_unlocked,FILE *stream )
DCE_ALIAS (getchar,getchar_unlocked)
DCE_ALIAS (fputc,fputc_unlocked)
NATIVE (putc, int c, FILE *stream)
NATIVE (putc_unlocked, int c, FILE *stream)
DCE_ALIAS (putchar, putchar_unlocked)
DCE_ALIAS (fgets, fgets_unlocked)
DCE_ALIAS (fputs, fputs_unlocked)
DCE_ALIAS (fread, fread_unlocked)
DCE_ALIAS (fwrite,fwrite_unlocked)
DCE_ALIAS (fflush,fflush_unlocked)
DCE_ALIAS (ferror,ferror_unlocked)
DCE_ALIAS (feof,feof_unlocked)
DCE_ALIAS (fileno,fileno_unlocked)
//NATIVE_WITH_ALIAS2 (sscanf, __isoc99_sscanf)

// REGRESSION
//NATIVE (__cmsg_nxthdr, void *__ctl, __kernel_size_t __size, struct cmsghdr *__cmsg)
NATIVE (flockfile, FILE *filehandle)
NATIVE (funlockfile, FILE *filehandle)

#endif /* SIMU_STDIO_H */
