/* stats_wrapper.c -- Wrapper to compile stats.c for bare-metal
 *
 * The original c/version.h unconditionally defines USE_DEV_URANDOM_UUID
 * which makes S_unique_id() try to open /dev/urandom (not available
 * on bare-metal). We undef it here so stats.c falls through to the
 * uuid_generate() path, which our libc/include/uuid/uuid.h stubs.
 */
#undef USE_DEV_URANDOM_UUID
#include "stats.c"
