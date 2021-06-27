#include "a6osx/c/types.h"
#include "a6osx/boot/a6osx/scheme.h"
#include "a6osx/c/main.c"
int main(int argc, const char *argv[]);
  void Sscheme_init(void (*abnormal_exit) PROTO((void)));
  void Sbuild_heap(const char *kernel, void (*custom_init) PROTO((void)));
  ptr Scall1(ptr cp, ptr x1);
  INT Sscheme_start(INT argc, const char *argv[]);
  void Sscheme_deinit();