#!/bin/sh
set -e

opts="-Wall -O0 -g -Wno-missing-braces -Wno-unused-function -Wno-null-dereference"
code="$(pwd)/src"
libs="-L /usr/local/lib -L /opt/homebrew/lib"
rpaths="-rpath /usr/local/lib -rpath /opt/homebrew/lib"

mkdir -p build
cd build

# compile program
clang \
	-arch arm64 \
	$opts "$code/sdl/sdl_main.c" \
	-I"$code/" \
	-c -o app.o

# compile vulkan memory allocator seperately
if [ ! -f "vk_mem_alloc.o" ]; then
	clang++ \
		-arch arm64 \
		$opts "$code/ext/vk_mem_alloc.cpp" \
		-w \
		-std=c++20 \
		-c -o vk_mem_alloc.o
fi

# link together
clang++ \
	-arch arm64 \
	app.o vk_mem_alloc.o \
	$libs \
	$rpaths \
	-lSDL3 \
	-lvolk \
	-o mgp_macos

cd -
