/* expeditor_stub.c -- Stub for expeditor (expression editor)
 * Not available on bare-metal -- no curses/terminal support.
 * Registers dummy foreign functions so boot file loading doesn't fail.
 */

#include "system.h"

/* Dummy implementations - ee_init_term returns 0 (false) which tells
 * Chez Scheme the terminal is not available, so it falls back to
 * basic line-oriented I/O. */
static IBOOL s_ee_init_term(iptr in, iptr out) {
    (void)in; (void)out;
    return 0;
}

static int s_ee_read_char(IBOOL block) { (void)block; return -1; }
static IBOOL s_ee_pending_winch(void) { return 0; }
static void s_ee_write_char(int c) { (void)c; }
static int s_ee_char_width(int c) { (void)c; return 1; }
static void s_ee_set_color(int c) { (void)c; }
static void s_ee_flush(void) {}
static void s_ee_get_screen_size(int *rows, int *cols) { *rows = 24; *cols = 80; }
static void s_ee_raw(void) {}
static void s_ee_noraw(void) {}
static void s_ee_postoutput(void) {}
static void s_ee_nopostoutput(void) {}
static void s_ee_signal(void) {}
static void s_ee_nosignal(void) {}
static void s_ee_enter_am_mode(void) {}
static void s_ee_exit_am_mode(void) {}
static void s_ee_pause(void) {}
static void s_ee_nanosleep(uptr ns) { (void)ns; }
static ptr s_ee_get_clipboard(void) { return Sfalse; }
static void s_ee_up(int n) { (void)n; }
static void s_ee_down(int n) { (void)n; }
static void s_ee_left(int n) { (void)n; }
static void s_ee_right(int n) { (void)n; }
static void s_ee_clear_eol(void) {}
static void s_ee_clear_eos(void) {}
static void s_ee_clear_screen(void) {}
static void s_ee_scroll_reverse(int n) { (void)n; }
static void s_ee_bell(void) {}
static void s_ee_carriage_return(void) {}
static void s_ee_line_feed(void) {}

void S_expeditor_init(void) {
    Sforeign_symbol("(cs)ee_init_term", (void *)s_ee_init_term);
    Sforeign_symbol("(cs)ee_read_char", (void *)s_ee_read_char);
    Sforeign_symbol("(cs)ee_pending_winch", (void *)s_ee_pending_winch);
    Sforeign_symbol("(cs)ee_write_char", (void *)s_ee_write_char);
    Sforeign_symbol("(cs)ee_char_width", (void *)s_ee_char_width);
    Sforeign_symbol("(cs)ee_set_color", (void *)s_ee_set_color);
    Sforeign_symbol("(cs)ee_flush", (void *)s_ee_flush);
    Sforeign_symbol("(cs)ee_get_screen_size", (void *)s_ee_get_screen_size);
    Sforeign_symbol("(cs)ee_raw", (void *)s_ee_raw);
    Sforeign_symbol("(cs)ee_noraw", (void *)s_ee_noraw);
    Sforeign_symbol("(cs)ee_postoutput", (void *)s_ee_postoutput);
    Sforeign_symbol("(cs)ee_nopostoutput", (void *)s_ee_nopostoutput);
    Sforeign_symbol("(cs)ee_signal", (void *)s_ee_signal);
    Sforeign_symbol("(cs)ee_nosignal", (void *)s_ee_nosignal);
    Sforeign_symbol("(cs)ee_enter_am_mode", (void *)s_ee_enter_am_mode);
    Sforeign_symbol("(cs)ee_exit_am_mode", (void *)s_ee_exit_am_mode);
    Sforeign_symbol("(cs)ee_pause", (void *)s_ee_pause);
    Sforeign_symbol("(cs)ee_nanosleep", (void *)s_ee_nanosleep);
    Sforeign_symbol("(cs)ee_get_clipboard", (void *)s_ee_get_clipboard);
    Sforeign_symbol("(cs)ee_up", (void *)s_ee_up);
    Sforeign_symbol("(cs)ee_down", (void *)s_ee_down);
    Sforeign_symbol("(cs)ee_left", (void *)s_ee_left);
    Sforeign_symbol("(cs)ee_right", (void *)s_ee_right);
    Sforeign_symbol("(cs)ee_clr_eol", (void *)s_ee_clear_eol);
    Sforeign_symbol("(cs)ee_clr_eos", (void *)s_ee_clear_eos);
    Sforeign_symbol("(cs)ee_clear_screen", (void *)s_ee_clear_screen);
    Sforeign_symbol("(cs)ee_scroll_reverse", (void *)s_ee_scroll_reverse);
    Sforeign_symbol("(cs)ee_bell", (void *)s_ee_bell);
    Sforeign_symbol("(cs)ee_carriage_return", (void *)s_ee_carriage_return);
    Sforeign_symbol("(cs)ee_line_feed", (void *)s_ee_line_feed);
}
