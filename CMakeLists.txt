cmake_minimum_required(VERSION 3.15)

project(ChezScheme)

set(CMAKE_VERBOSE_MAKEFILE on)

set(compile_options -m64 -Wpointer-arith -Wall -Wextra -Wno-implicit-fallthrough -Werror -O0 -DX86_64)
set(include_directories a6osx/boot/a6osx a6osx/zlib a6osx/lz4/lib /opt/X11/include/ a6osx/c/)

add_library(kernel OBJECT 
        a6osx/c/statics.c a6osx/c/segment.c a6osx/c/alloc.c 
        a6osx/c/symbol.c a6osx/c/intern.c a6osx/c/gcwrapper.c 
        a6osx/c/gc-011.c a6osx/c/gc-ocd.c a6osx/c/gc-oce.c 
        a6osx/c/number.c a6osx/c/schsig.c a6osx/c/io.c 
        a6osx/c/new-io.c a6osx/c/print.c a6osx/c/fasl.c 
        a6osx/c/stats.c a6osx/c/foreign.c a6osx/c/prim.c a6osx/c/prim5.c 
        a6osx/c/flushcache.c a6osx/c/schlib.c a6osx/c/thread.c 
        a6osx/c/expeditor.c a6osx/c/scheme.c a6osx/c/compress-io.c a6osx/c/i3le.c)
target_link_libraries(kernel
        ${CMAKE_SOURCE_DIR}/a6osx/lz4/lib/liblz4.a 
        ${CMAKE_SOURCE_DIR}/a6osx/zlib/libz.a)
target_compile_options(kernel PUBLIC ${compile_options})
target_include_directories(kernel PUBLIC ${include_directories})

add_executable(scheme a6osx/c/main.c)
target_link_libraries(scheme
        kernel
        # shared library 等价于给连接器传 -liconv -lm -lncurses
        iconv m ncurses)
target_compile_options(scheme PUBLIC ${compile_options})
target_include_directories(scheme PUBLIC ${include_directories})

add_library(key-functions OBJECT tin-note/key.h)

## lab

add_executable(scheme-lab tin-note/lab.cpp)
#target_compile_options()
