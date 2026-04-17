/* stdio.h -- Standard I/O for bare-metal RV64G */

#ifndef _BAREMETAL_STDIO_H
#define _BAREMETAL_STDIO_H

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>

#define EOF         (-1)
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#define BUFSIZ      1024
#define FILENAME_MAX 4096
#define FOPEN_MAX   20
#define TMP_MAX     10000
#define L_tmpnam    20

#define _IOFBF      0   /* Full buffering */
#define _IOLBF      1   /* Line buffering */
#define _IONBF      2   /* No buffering */

/* Minimal FILE structure */
typedef struct _FILE {
    int     fd;
    int     flags;
    int     eof;
    int     error;
    int     ungetc_buf;     /* -1 if empty */
    char   *buf;
    size_t  buf_size;
    size_t  buf_pos;
    size_t  buf_len;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* Standard I/O functions */
FILE   *fopen(const char *path, const char *mode);
FILE   *fdopen(int fd, const char *mode);
FILE   *freopen(const char *path, const char *mode, FILE *stream);
int     fclose(FILE *stream);

size_t  fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t  fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int     fflush(FILE *stream);

int     fseek(FILE *stream, long offset, int whence);
long    ftell(FILE *stream);
void    rewind(FILE *stream);
int     fgetpos(FILE *stream, long *pos);
int     fsetpos(FILE *stream, const long *pos);

int     feof(FILE *stream);
int     ferror(FILE *stream);
void    clearerr(FILE *stream);
int     fileno(FILE *stream);

int     fprintf(FILE *stream, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int     printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int     sprintf(char *str, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int     snprintf(char *str, size_t size, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

int     vfprintf(FILE *stream, const char *fmt, va_list ap);
int     vprintf(const char *fmt, va_list ap);
int     vsprintf(char *str, const char *fmt, va_list ap);
int     vsnprintf(char *str, size_t size, const char *fmt, va_list ap);

int     fputc(int c, FILE *stream);
int     fputs(const char *s, FILE *stream);
int     fgetc(FILE *stream);
char   *fgets(char *s, int size, FILE *stream);

int     putc(int c, FILE *stream);
int     putchar(int c);
int     puts(const char *s);
int     getc(FILE *stream);
int     getchar(void);
int     ungetc(int c, FILE *stream);

void    perror(const char *s);
int     remove(const char *pathname);

int     setvbuf(FILE *stream, char *buf, int mode, size_t size);
void    setbuf(FILE *stream, char *buf);

int     sscanf(const char *str, const char *fmt, ...);
int     fscanf(FILE *stream, const char *fmt, ...);
int     scanf(const char *fmt, ...);

FILE   *tmpfile(void);
char   *tmpnam(char *s);

FILE   *popen(const char *command, const char *type);
int     pclose(FILE *stream);

#endif /* _BAREMETAL_STDIO_H */
