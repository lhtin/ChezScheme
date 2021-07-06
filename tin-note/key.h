#include "../a6osx/c/types.h"
#include "../a6osx/boot/a6osx/scheme.h"
#include "../a6osx/c/main.c"

int main(int argc, const char *argv[]);
  const char *Skernel_version(void);
  void Sscheme_init(void (*abnormal_exit) PROTO((void)));
  void Sregister_boot_file(const char *name);
  /* must call Sbuild_heap after registering boot and heap files.
  * Sbuild_heap() completes the initialization of the Scheme system
  * and loads the boot or heap files.  If no boot or heap files have
  * been registered, the first argument to Sbuild_heap must be a
  * non-null path string; in this case, Sbuild_heap looks for
  * a heap or boot file named <name>.boot, where <name> is the last
  * component of the path.  If no heap files are loaded and
  * CUSTOM_INIT is non-null, Sbuild_heap calls CUSTOM_INIT just
  * prior to loading the boot file(s). */
  void Sbuild_heap(const char *kernel, void (*custom_init) PROTO((void)));
    static void main_init();
  ptr Scall1(ptr cp, ptr x1);
  INT Sscheme_start(INT argc, const char *argv[]);
  void Sscheme_deinit();