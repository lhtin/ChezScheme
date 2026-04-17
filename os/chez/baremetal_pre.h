/* baremetal_pre.h -- Injected before all Chez Scheme C files via -include
 *
 * This header is force-included before everything else. It:
 * 1. Undefs OS macros so no OS-specific block in version.h activates
 * 2. Defines the bare-metal platform macros that version.h's
 *    "Defaults and derived" section needs
 *
 * This way we don't modify the original c/version.h at all.
 */

#ifndef BAREMETAL_PRE_H
#define BAREMETAL_PRE_H

/* --- Suppress all OS detection --- */
#undef __linux__
#undef __linux
#undef __gnu_linux__
#undef __FreeBSD__
#undef __FreeBSD_kernel__
#undef __NetBSD__
#undef __OpenBSD__
#undef __APPLE__
#undef __EMSCRIPTEN__
#undef __QNX__
#undef __COSMOPOLITAN__
#undef __GNU__
#undef _MSC_VER
#undef __MINGW32__
#undef WIN32
#undef sun

/* --- Bare-metal platform definitions ---
 * These are normally set by an OS block in version.h.
 * Since no OS block will activate, we define them here. */
#define __BAREMETAL_RV64__

#define USE_MALLOC
#define GETPAGESIZE() 4096
#define NOBLOCK 0x800
#define IEEE_DOUBLE
#define LDEXP
#define ARCHYPERBOLIC
typedef char *memcpy_t;
#define MAKE_NAN(x) { x = 0.0; x = x / x; }
#define GETWD(x) getcwd((x), 4096)
typedef int tputsputcchar;
#define DIRMARKERP(c) ((c) == '/')
#define LSEEK lseek
#define OFF_T long
#define SECATIME(sb) (sb).st_atim.tv_sec
#define SECCTIME(sb) (sb).st_ctim.tv_sec
#define SECMTIME(sb) (sb).st_mtim.tv_sec
#define NSECATIME(sb) (sb).st_atim.tv_nsec
#define NSECCTIME(sb) (sb).st_ctim.tv_nsec
#define NSECMTIME(sb) (sb).st_mtim.tv_nsec
#define ICONV_INBUF_TYPE char **
#define NO_USELOCALE
#define DISABLE_X11

/* Disable features not available on bare-metal */
/* (LOAD_SHARED_OBJECT and LOCKF are not defined, so already disabled) */

#endif /* BAREMETAL_PRE_H */
