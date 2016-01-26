#!/bin/bash
#
# Test build script
#
CC=/usr/bin/cc
CFLAGS="-fPIC -shared -std=c++11"
$CC $CFLAGS -o libunionclientlibrary.so -Iinclude src/*.cpp

