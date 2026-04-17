/* stdio.c -- ChezSchemeOS freestanding libc: FILE and POSIX fd operations
 *
 * fd 0/1/2 are routed to UART. All other fds return errors.
 */

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

extern void uart_putc(char c);
extern char uart_getc(void);
extern int  uart_getc_nonblock(void);
extern void uart_puts(const char *s);

/* Timer callback support — defined in timer.c */
extern int  timer_has_pending(void);
extern void timer_run_pending(void);
extern void *memset(void *, int, size_t);
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);

/* errno */
extern int errno;
#define EBADF   9
#define ENOENT  2
#define ESPIPE  29
#define EINVAL  22
#define ENOMEM  12

/* FILE structure */
typedef struct _FILE {
    int fd;
    int eof;
    int error;
    int mode;  /* 0=read, 1=write, 2=rw */
} FILE;

static FILE _stdin_obj  = { 0, 0, 0, 0 };
static FILE _stdout_obj = { 1, 0, 0, 1 };
static FILE _stderr_obj = { 2, 0, 0, 1 };

FILE *stdin  = &_stdin_obj;
FILE *stdout = &_stdout_obj;
FILE *stderr = &_stderr_obj;

/* POSIX-level fd operations */

typedef long ssize_t;
typedef long off_t;

/* Line buffer for stdin - collect until Enter is pressed */
static char line_buf[4096];
static int line_len = 0;    /* total chars in buffer */
static int line_pos = 0;    /* current read position */
static int line_ready = 0;  /* 1 = line has been entered, data available */

/* Command history ring buffer */
#define HIST_MAX 64
#define HIST_LINE_MAX 1024
static char hist_ring[HIST_MAX][HIST_LINE_MAX];
static int hist_count = 0;   /* total entries stored */
static int hist_head = 0;    /* next write slot */
static int hist_browse = 0;  /* current browse position */
static char hist_saved[HIST_LINE_MAX]; /* saved current line when browsing */
static int hist_saved_len = 0;

static void hist_add(const char *s, int len) {
    /* Don't add empty lines or duplicates */
    if (len <= 0) return;
    if (len == 1 && s[0] == '\n') return;
    /* Strip trailing newline for storage */
    int slen = (s[len - 1] == '\n') ? len - 1 : len;
    if (slen <= 0) return;
    /* Skip duplicate of last entry */
    if (hist_count > 0) {
        int prev = (hist_head - 1 + HIST_MAX) % HIST_MAX;
        int plen = 0;
        while (hist_ring[prev][plen]) plen++;
        if (plen == slen) {
            int same = 1;
            for (int i = 0; i < slen; i++)
                if (hist_ring[prev][i] != s[i]) { same = 0; break; }
            if (same) return;
        }
    }
    int n = slen < HIST_LINE_MAX - 1 ? slen : HIST_LINE_MAX - 1;
    for (int i = 0; i < n; i++) hist_ring[hist_head][i] = s[i];
    hist_ring[hist_head][n] = '\0';
    hist_head = (hist_head + 1) % HIST_MAX;
    if (hist_count < HIST_MAX) hist_count++;
}

/* Replace the current line_buf/line_len with a history entry, update screen */
static void replace_line(int *cursor, const char *s) {
    int old_len = line_len;
    /* Move cursor to start */
    while (*cursor > 0) { (*cursor)--; uart_putc('\b'); }
    /* Copy new content */
    line_len = 0;
    while (s[line_len] && line_len < (int)sizeof(line_buf) - 2)  {
        line_buf[line_len] = s[line_len];
        line_len++;
    }
    /* Print new line, erase old extra chars */
    for (int i = 0; i < line_len; i++) uart_putc(line_buf[i]);
    for (int i = line_len; i < old_len; i++) uart_putc(' ');
    for (int i = line_len; i < old_len; i++) uart_putc('\b');
    *cursor = line_len;
}

/* --- UTF-8 helpers --- */

/* Return the byte length of the UTF-8 character starting at buf[pos] */
static int utf8_char_len(const char *buf, int pos, int len) {
    unsigned char c = (unsigned char)buf[pos];
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return (pos + 2 <= len) ? 2 : 1;
    if ((c & 0xF0) == 0xE0) return (pos + 3 <= len) ? 3 : 1;
    if ((c & 0xF8) == 0xF0) return (pos + 4 <= len) ? 4 : 1;
    return 1; /* invalid, treat as 1 byte */
}

/* Return the byte length of the UTF-8 character ending at buf[pos-1]
 * (i.e., find the start of the character that contains byte pos-1) */
static int utf8_prev_char_len(const char *buf, int pos) {
    if (pos <= 0) return 0;
    int back = 1;
    /* Walk back over continuation bytes (10xxxxxx) */
    while (back < pos && back < 4 && ((unsigned char)buf[pos - back] & 0xC0) == 0x80)
        back++;
    return back;
}

/* Return the display width of a UTF-8 character.
 * CJK characters (U+2E80..U+9FFF, U+F900..U+FAFF, U+FE30..U+FE4F,
 * U+FF00..U+FF60, U+FFE0..U+FFE6, U+20000..U+2FA1F) are width 2.
 * Most others are width 1. */
static int utf8_display_width(const char *buf, int pos, int len) {
    unsigned char c = (unsigned char)buf[pos];
    if (c < 0x80) return 1;
    /* Decode the codepoint */
    unsigned int cp = 0;
    int clen = utf8_char_len(buf, pos, len);
    if (clen == 2) {
        cp = (c & 0x1F) << 6;
        cp |= ((unsigned char)buf[pos+1] & 0x3F);
    } else if (clen == 3) {
        cp = (c & 0x0F) << 12;
        cp |= ((unsigned char)buf[pos+1] & 0x3F) << 6;
        cp |= ((unsigned char)buf[pos+2] & 0x3F);
    } else if (clen == 4) {
        cp = (c & 0x07) << 18;
        cp |= ((unsigned char)buf[pos+1] & 0x3F) << 12;
        cp |= ((unsigned char)buf[pos+2] & 0x3F) << 6;
        cp |= ((unsigned char)buf[pos+3] & 0x3F);
    } else {
        return 1;
    }
    /* CJK wide characters */
    if ((cp >= 0x1100 && cp <= 0x115F) ||   /* Hangul Jamo */
        cp == 0x2329 || cp == 0x232A ||
        (cp >= 0x2E80 && cp <= 0x303E) ||   /* CJK Radicals, Kangxi, CJK Symbols */
        (cp >= 0x3040 && cp <= 0x33BF) ||   /* Hiragana, Katakana, Bopomofo, CJK Compat */
        (cp >= 0x3400 && cp <= 0x4DBF) ||   /* CJK Unified Ext A */
        (cp >= 0x4E00 && cp <= 0x9FFF) ||   /* CJK Unified */
        (cp >= 0xA000 && cp <= 0xA4CF) ||   /* Yi */
        (cp >= 0xAC00 && cp <= 0xD7AF) ||   /* Hangul Syllables */
        (cp >= 0xF900 && cp <= 0xFAFF) ||   /* CJK Compat Ideographs */
        (cp >= 0xFE10 && cp <= 0xFE6F) ||   /* CJK Compat Forms, Small Forms */
        (cp >= 0xFF01 && cp <= 0xFF60) ||   /* Fullwidth Forms */
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||   /* Fullwidth Signs */
        (cp >= 0x20000 && cp <= 0x2FA1F))   /* CJK Ext B-F, Compat Supplement */
        return 2;
    return 1;
}

/* Calculate the total display width of line_buf[from..to) */
static int display_width_range(int from, int to) {
    int w = 0;
    int i = from;
    while (i < to) {
        w += utf8_display_width(line_buf, i, to);
        i += utf8_char_len(line_buf, i, to);
    }
    return w;
}

/* Redraw from byte position 'cursor' to end, erase old trailing display chars, reposition cursor.
 * old_dw = old display width from cursor to old end */
static void redraw_from_utf8(int cursor, int old_dw) {
    /* Print from cursor to end */
    for (int i = cursor; i < line_len; i++)
        uart_putc(line_buf[i]);
    int new_dw = display_width_range(cursor, line_len);
    /* Erase leftover display chars */
    for (int i = new_dw; i < old_dw; i++)
        uart_putc(' ');
    /* Move cursor back */
    int back = (old_dw > new_dw ? old_dw : new_dw);
    for (int i = 0; i < back; i++)
        uart_putc('\b');
}

/* Move terminal cursor left by the display width of one UTF-8 char */
static void cursor_left_by(int dw) {
    for (int i = 0; i < dw; i++) uart_putc('\b');
}

/* Output one UTF-8 character from line_buf at position pos */
static void emit_utf8_char(int pos) {
    int clen = utf8_char_len(line_buf, pos, line_len);
    for (int i = 0; i < clen; i++)
        uart_putc(line_buf[pos + i]);
}

/* Global prompt string — updated by REPL manager */
static char current_prompt[32] = "> ";

void stdio_set_prompt(const char *prompt) {
    int i = 0;
    while (prompt[i] && i < 30) { current_prompt[i] = prompt[i]; i++; }
    current_prompt[i] = '\0';
}

static int prompt_display_width(void) {
    int w = 0;
    for (int i = 0; current_prompt[i]; i++) w++;
    return w;
}

/* Collect a line from UART into line_buf (blocking until Enter) */
static void collect_line(void) {
    int cursor = 0;    /* byte position in line_buf */
    line_len = 0;
    line_pos = 0;
    line_ready = 0;
    hist_browse = 0;
    hist_saved_len = 0;

    while (line_len < (int)sizeof(line_buf) - 4) {
        /* Non-blocking check for input, run timer callbacks while waiting */
        int ic = uart_getc_nonblock();
        if (ic < 0) {
            /* No input available — check for pending timer callbacks */
            if (timer_has_pending()) {
                /* Erase current line from screen */
                int cur_dw = display_width_range(0, cursor);
                cursor_left_by(cur_dw);
                int total_dw = display_width_range(0, line_len);
                for (int j = 0; j < total_dw; j++) uart_putc(' ');
                for (int j = 0; j < total_dw; j++) uart_putc('\b');
                /* Erase the prompt too */
                int pdw = prompt_display_width();
                for (int j = 0; j < pdw; j++) uart_putc('\b');
                for (int j = 0; j < pdw; j++) uart_putc(' ');
                for (int j = 0; j < pdw; j++) uart_putc('\b');

                /* Run callbacks (they may print output) */
                timer_run_pending();

                /* Redraw prompt and current line */
                uart_puts(current_prompt);
                for (int j = 0; j < line_len; j++) uart_putc(line_buf[j]);
                /* Move cursor back to position */
                int tail_dw = display_width_range(cursor, line_len);
                cursor_left_by(tail_dw);
            }
            /* Brief pause to avoid busy-spinning */
            __asm__ volatile("nop; nop; nop; nop");
            continue;
        }
        char c = (char)ic;

        if (c == '\r' || c == '\n') {
            /* Enter */
            uart_putc('\r');
            uart_putc('\n');
            hist_add(line_buf, line_len);
            line_buf[line_len++] = '\n';
            break;

        } else if (c == 127 || c == '\b') {
            /* Backspace: delete one UTF-8 char before cursor */
            if (cursor > 0) {
                int clen = utf8_prev_char_len(line_buf, cursor);
                int dw = utf8_display_width(line_buf, cursor - clen, line_len);
                int old_dw = display_width_range(cursor, line_len);
                /* Remove clen bytes at cursor-clen */
                for (int i = cursor - clen; i < line_len - clen; i++)
                    line_buf[i] = line_buf[i + clen];
                line_len -= clen;
                cursor -= clen;
                /* Move terminal cursor back */
                cursor_left_by(dw);
                /* Redraw rest */
                redraw_from_utf8(cursor, old_dw + dw);
            }

        } else if (c == 0x1b) {
            /* ESC sequence */
            char s1 = uart_getc();
            if (s1 == '[') {
                char s2 = uart_getc();
                if (s2 == 'A') {
                    /* Up arrow: previous history */
                    if (hist_browse < hist_count) {
                        if (hist_browse == 0) {
                            for (int i = 0; i < line_len && i < HIST_LINE_MAX - 1; i++)
                                hist_saved[i] = line_buf[i];
                            hist_saved[line_len < HIST_LINE_MAX - 1 ? line_len : HIST_LINE_MAX - 1] = '\0';
                            hist_saved_len = line_len;
                        }
                        hist_browse++;
                        int idx = (hist_head - hist_browse + HIST_MAX) % HIST_MAX;
                        replace_line(&cursor, hist_ring[idx]);
                    }
                } else if (s2 == 'B') {
                    /* Down arrow */
                    if (hist_browse > 0) {
                        hist_browse--;
                        if (hist_browse == 0) {
                            hist_saved[hist_saved_len] = '\0';
                            replace_line(&cursor, hist_saved);
                            line_len = hist_saved_len;
                        } else {
                            int idx = (hist_head - hist_browse + HIST_MAX) % HIST_MAX;
                            replace_line(&cursor, hist_ring[idx]);
                        }
                    }
                } else if (s2 == 'C') {
                    /* Right arrow: move one UTF-8 char right */
                    if (cursor < line_len) {
                        int clen = utf8_char_len(line_buf, cursor, line_len);
                        emit_utf8_char(cursor);
                        cursor += clen;
                    }
                } else if (s2 == 'D') {
                    /* Left arrow: move one UTF-8 char left */
                    if (cursor > 0) {
                        int clen = utf8_prev_char_len(line_buf, cursor);
                        int dw = utf8_display_width(line_buf, cursor - clen, line_len);
                        cursor -= clen;
                        cursor_left_by(dw);
                    }
                } else if (s2 == 'H') {
                    /* Home */
                    int dw = display_width_range(0, cursor);
                    cursor_left_by(dw);
                    cursor = 0;
                } else if (s2 == 'F') {
                    /* End */
                    while (cursor < line_len) {
                        emit_utf8_char(cursor);
                        cursor += utf8_char_len(line_buf, cursor, line_len);
                    }
                } else if (s2 >= '0' && s2 <= '9') {
                    char s3 = uart_getc();
                    if (s3 == '~') {
                        if (s2 == '3' && cursor < line_len) {
                            /* Delete: remove UTF-8 char at cursor */
                            int clen = utf8_char_len(line_buf, cursor, line_len);
                            int old_dw = display_width_range(cursor, line_len);
                            for (int i = cursor; i < line_len - clen; i++)
                                line_buf[i] = line_buf[i + clen];
                            line_len -= clen;
                            redraw_from_utf8(cursor, old_dw);
                        } else if (s2 == '1') {
                            int dw = display_width_range(0, cursor);
                            cursor_left_by(dw);
                            cursor = 0;
                        } else if (s2 == '4') {
                            while (cursor < line_len) {
                                emit_utf8_char(cursor);
                                cursor += utf8_char_len(line_buf, cursor, line_len);
                            }
                        }
                    }
                }
            }

        } else if (c == 0x01) {
            /* Ctrl-A */
            int dw = display_width_range(0, cursor);
            cursor_left_by(dw);
            cursor = 0;
        } else if (c == 0x05) {
            /* Ctrl-E */
            while (cursor < line_len) {
                emit_utf8_char(cursor);
                cursor += utf8_char_len(line_buf, cursor, line_len);
            }
        } else if (c == 0x0b) {
            /* Ctrl-K */
            int old_dw = display_width_range(cursor, line_len);
            line_len = cursor;
            redraw_from_utf8(cursor, old_dw);
        } else if (c == 0x15) {
            /* Ctrl-U */
            int dw = display_width_range(0, cursor);
            cursor_left_by(dw);
            int old_dw = display_width_range(0, line_len);
            line_len = 0;
            cursor = 0;
            redraw_from_utf8(0, old_dw);
        } else if (c == 0x03) {
            /* Ctrl-C */
            uart_puts("^C\r\n");
            line_len = 0;
            line_buf[line_len++] = '\n';
            break;
        } else if (c == 0x0e) {
            /* Ctrl-N: switch to next REPL */
            /* Erase current line, inject (next-repl) command */
            int cur_dw = display_width_range(0, cursor);
            cursor_left_by(cur_dw);
            int total_dw = display_width_range(0, line_len);
            for (int j = 0; j < total_dw; j++) uart_putc(' ');
            for (int j = 0; j < total_dw; j++) uart_putc('\b');
            const char *cmd = "(next-repl)";
            line_len = 0;
            while (*cmd) { line_buf[line_len++] = *cmd; uart_putc(*cmd); cmd++; }
            uart_putc('\r'); uart_putc('\n');
            line_buf[line_len++] = '\n';
            break;
        } else if (c == 0x10) {
            /* Ctrl-P: switch to previous REPL */
            int cur_dw = display_width_range(0, cursor);
            cursor_left_by(cur_dw);
            int total_dw = display_width_range(0, line_len);
            for (int j = 0; j < total_dw; j++) uart_putc(' ');
            for (int j = 0; j < total_dw; j++) uart_putc('\b');
            const char *cmd = "(prev-repl)";
            line_len = 0;
            while (*cmd) { line_buf[line_len++] = *cmd; uart_putc(*cmd); cmd++; }
            uart_putc('\r'); uart_putc('\n');
            line_buf[line_len++] = '\n';
            break;
        } else if (c == 0x04) {
            /* Ctrl-D */
            if (line_len == 0) { line_ready = -1; return; }
            line_buf[line_len++] = '\n';
            break;
        } else if ((unsigned char)c >= 0x80) {
            /* UTF-8 multi-byte character: read remaining bytes */
            int total_bytes = 1;
            if (((unsigned char)c & 0xE0) == 0xC0) total_bytes = 2;
            else if (((unsigned char)c & 0xF0) == 0xE0) total_bytes = 3;
            else if (((unsigned char)c & 0xF8) == 0xF0) total_bytes = 4;

            char mb[4];
            mb[0] = c;
            for (int i = 1; i < total_bytes; i++)
                mb[i] = uart_getc();

            if (line_len + total_bytes < (int)sizeof(line_buf) - 1) {
                /* Insert at cursor */
                for (int i = line_len - 1 + total_bytes; i >= cursor + total_bytes; i--)
                    line_buf[i] = line_buf[i - total_bytes];
                for (int i = 0; i < total_bytes; i++)
                    line_buf[cursor + i] = mb[i];
                line_len += total_bytes;
                /* Print from cursor to end */
                for (int i = cursor; i < line_len; i++)
                    uart_putc(line_buf[i]);
                cursor += total_bytes;
                /* Move cursor back to position */
                int tail_dw = display_width_range(cursor, line_len);
                cursor_left_by(tail_dw);
            }
        } else if (c >= 0x20 || c == '\t') {
            /* ASCII printable */
            if (line_len < (int)sizeof(line_buf) - 1) {
                for (int i = line_len; i > cursor; i--)
                    line_buf[i] = line_buf[i - 1];
                line_buf[cursor] = c;
                line_len++;
                for (int i = cursor; i < line_len; i++)
                    uart_putc(line_buf[i]);
                cursor++;
                int tail_dw = display_width_range(cursor, line_len);
                cursor_left_by(tail_dw);
            }
        }
    }
    line_ready = 1;
}

ssize_t read(int fd, void *buf, size_t count) {
    if (fd == 0) {
        char *p = (char *)buf;
        if (count == 0) return 0;

        /* If buffer is empty, collect a new line */
        while (line_pos >= line_len) {
            collect_line();
            if (line_ready < 0) return 0; /* EOF */
            /* If user pressed Enter on an empty line, reprint prompt and retry */
            if (line_len == 1 && line_buf[0] == '\n') {
                uart_puts(current_prompt);
                line_pos = 0;
                line_len = 0;
                continue;
            }
            break;
        }

        /* Return as much as we can from the buffer */
        size_t avail = (size_t)(line_len - line_pos);
        size_t n = (count < avail) ? count : avail;
        for (size_t i = 0; i < n; i++) {
            p[i] = line_buf[line_pos++];
        }
        return (ssize_t)n;
    }
    errno = EBADF;
    return -1;
}

ssize_t write(int fd, const void *buf, size_t count) {
    if (fd == 1 || fd == 2) {
        const char *p = (const char *)buf;
        for (size_t i = 0; i < count; i++) {
            if (p[i] == '\n') uart_putc('\r');
            uart_putc(p[i]);
        }
        return (ssize_t)count;
    }
    errno = EBADF;
    return -1;
}

int open(const char *pathname, int flags, ...) {
    (void)pathname;
    (void)flags;
    errno = ENOENT;
    return -1;
}

int close(int fd) {
    if (fd >= 0 && fd <= 2) return 0;
    errno = EBADF;
    return -1;
}

off_t lseek(int fd, off_t offset, int whence) {
    (void)fd; (void)offset; (void)whence;
    errno = ESPIPE;
    return -1;
}

/* Also provide lseek64 alias */
off_t lseek64(int fd, off_t offset, int whence) {
    return lseek(fd, offset, whence);
}

/* FILE-level operations */

FILE *fopen(const char *path, const char *mode) {
    (void)path; (void)mode;
    errno = ENOENT;
    return NULL;
}

FILE *fdopen(int fd, const char *mode) {
    (void)mode;
    if (fd == 0) return stdin;
    if (fd == 1) return stdout;
    if (fd == 2) return stderr;
    errno = EBADF;
    return NULL;
}

int fclose(FILE *stream) {
    if (stream == stdin || stream == stdout || stream == stderr) return 0;
    return -1;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (stream->fd == 0) {
        size_t total = size * nmemb;
        ssize_t n = read(0, ptr, total);
        if (n <= 0) { stream->eof = 1; return 0; }
        return (size_t)n / size;
    }
    stream->error = 1;
    return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (stream->fd == 1 || stream->fd == 2) {
        size_t total = size * nmemb;
        ssize_t n = write(stream->fd, ptr, total);
        if (n < 0) { stream->error = 1; return 0; }
        return (size_t)n / size;
    }
    stream->error = 1;
    return 0;
}

int fputc(int c, FILE *stream) {
    if (stream->fd == 1 || stream->fd == 2) {
        if (c == '\n') uart_putc('\r');
        uart_putc((char)c);
        return c;
    }
    return -1;
}

int fputs(const char *s, FILE *stream) {
    while (*s) {
        if (fputc(*s, stream) == -1) return -1;
        s++;
    }
    return 0;
}

int fgetc(FILE *stream) {
    if (stream->fd == 0) {
        char c = uart_getc();
        if (c == '\r') c = '\n';
        return (unsigned char)c;
    }
    stream->eof = 1;
    return -1;
}

char *fgets(char *s, int size, FILE *stream) {
    if (size <= 0) return NULL;
    int i = 0;
    while (i < size - 1) {
        int c = fgetc(stream);
        if (c == -1) {
            if (i == 0) return NULL;
            break;
        }
        s[i++] = (char)c;
        if (c == '\n') break;
    }
    s[i] = '\0';
    return s;
}

int putc(int c, FILE *stream) {
    return fputc(c, stream);
}

int putchar(int c) {
    return fputc(c, stdout);
}

int puts(const char *s) {
    fputs(s, stdout);
    fputc('\n', stdout);
    return 0;
}

int getc(FILE *stream) {
    return fgetc(stream);
}

int getchar(void) {
    return fgetc(stdin);
}

int ungetc(int c, FILE *stream) {
    /* Minimal stub -- not properly implemented */
    (void)c; (void)stream;
    return c;
}

int fseek(FILE *stream, long offset, int whence) {
    (void)stream; (void)offset; (void)whence;
    errno = ESPIPE;
    return -1;
}

long ftell(FILE *stream) {
    (void)stream;
    errno = ESPIPE;
    return -1;
}

void rewind(FILE *stream) {
    (void)stream;
}

int feof(FILE *stream) {
    return stream->eof;
}

int ferror(FILE *stream) {
    return stream->error;
}

void clearerr(FILE *stream) {
    stream->eof = 0;
    stream->error = 0;
}

int fileno(FILE *stream) {
    return stream->fd;
}

int fflush(FILE *stream) {
    (void)stream;
    return 0; /* UART is unbuffered */
}

void perror(const char *s) {
    extern char *strerror(int);
    if (s && *s) {
        fputs(s, stderr);
        fputs(": ", stderr);
    }
    fputs(strerror(errno), stderr);
    fputc('\n', stderr);
}

int remove(const char *pathname) {
    (void)pathname;
    errno = ENOENT;
    return -1;
}

int rename(const char *oldpath, const char *newpath) {
    (void)oldpath; (void)newpath;
    errno = ENOENT;
    return -1;
}

/* setvbuf, setbuf -- no-op stubs */
int setvbuf(FILE *stream, char *buf, int mode, size_t size) {
    (void)stream; (void)buf; (void)mode; (void)size;
    return 0;
}

void setbuf(FILE *stream, char *buf) {
    (void)stream; (void)buf;
}

FILE *tmpfile(void) { return NULL; }
char *tmpnam(char *s) { (void)s; return NULL; }

/* Minimal sscanf implementation */
int vsscanf(const char *str, const char *fmt, va_list ap) {
    int count = 0;
    const char *s = str;

    while (*fmt && *s) {
        if (*fmt == ' ') {
            while (*s == ' ' || *s == '\t' || *s == '\n') s++;
            fmt++;
            continue;
        }
        if (*fmt != '%') {
            if (*s != *fmt) break;
            s++; fmt++;
            continue;
        }
        fmt++; /* skip % */

        /* Width */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        /* Length */
        int length = 0;
        if (*fmt == 'l') { fmt++; length = 1; if (*fmt == 'l') { fmt++; length = 2; } }
        else if (*fmt == 'h') { fmt++; if (*fmt == 'h') fmt++; }

        char spec = *fmt++;

        switch (spec) {
        case 'd': case 'i': {
            while (*s == ' ') s++;
            int neg = 0;
            if (*s == '-') { neg = 1; s++; }
            else if (*s == '+') s++;
            long long val = 0;
            int digits = 0;
            while (*s >= '0' && *s <= '9') {
                val = val * 10 + (*s - '0');
                s++; digits++;
                if (width && digits >= width) break;
            }
            if (digits == 0) goto done;
            if (neg) val = -val;
            if (length == 2) *va_arg(ap, long long *) = val;
            else if (length == 1) *va_arg(ap, long *) = (long)val;
            else *va_arg(ap, int *) = (int)val;
            count++;
            break;
        }
        case 'u': {
            while (*s == ' ') s++;
            unsigned long long val = 0;
            int digits = 0;
            while (*s >= '0' && *s <= '9') {
                val = val * 10 + (*s - '0');
                s++; digits++;
                if (width && digits >= width) break;
            }
            if (digits == 0) goto done;
            if (length == 2) *va_arg(ap, unsigned long long *) = val;
            else if (length == 1) *va_arg(ap, unsigned long *) = (unsigned long)val;
            else *va_arg(ap, unsigned int *) = (unsigned int)val;
            count++;
            break;
        }
        case 'x': case 'X': {
            while (*s == ' ') s++;
            if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
            unsigned long long val = 0;
            int digits = 0;
            while (1) {
                int d = -1;
                if (*s >= '0' && *s <= '9') d = *s - '0';
                else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
                else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
                else break;
                val = val * 16 + d;
                s++; digits++;
                if (width && digits >= width) break;
            }
            if (digits == 0) goto done;
            if (length == 2) *va_arg(ap, unsigned long long *) = val;
            else if (length == 1) *va_arg(ap, unsigned long *) = (unsigned long)val;
            else *va_arg(ap, unsigned int *) = (unsigned int)val;
            count++;
            break;
        }
        case 's': {
            while (*s == ' ') s++;
            char *dest = va_arg(ap, char *);
            int n = 0;
            while (*s && *s != ' ' && *s != '\t' && *s != '\n') {
                if (width && n >= width) break;
                *dest++ = *s++;
                n++;
            }
            *dest = '\0';
            if (n > 0) count++;
            else goto done;
            break;
        }
        case 'c': {
            char *dest = va_arg(ap, char *);
            *dest = *s++;
            count++;
            break;
        }
        case 'n': {
            *va_arg(ap, int *) = (int)(s - str);
            break;
        }
        default:
            goto done;
        }
    }
done:
    return count;
}

int sscanf(const char *str, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vsscanf(str, fmt, ap);
    va_end(ap);
    return ret;
}

int fscanf(FILE *stream, const char *fmt, ...) {
    /* Very basic: read a line, then sscanf it */
    (void)stream; (void)fmt;
    return 0; /* stub */
}

int scanf(const char *fmt, ...) {
    (void)fmt;
    return 0; /* stub */
}

