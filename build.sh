#!/bin/sh
set -e

opts="-Wall -O0 -g -Wno-missing-braces -Wno-unused-function -Wno-null-dereference"
code="$(pwd)/src"
libs="-L /usr/local/lib -L /opt/homebrew/lib"
rpaths="-rpath /usr/local/lib -rpath /opt/homebrew/lib -rpath ./build"

mkdir -p build
cd build

# compile vulkan memory allocator seperately

if [ ! -f "vk_mem_alloc.o" ]; then

	echo "Compiling Vulkan Memory Allocator..."

	clang++ \
		-arch arm64 \
		$opts "$code/ext/vk_mem_alloc.cpp" \
		-w \
		-fPIC \
		-std=c++20 \
		-c -o vk_mem_alloc.o
fi

# compile platform agnostic program
# seperately from platform layer to enable
# hot code reloading

echo "Compiling core..."

clang \
	-arch arm64 \
	$opts "$code/core.c" \
	-I"$code/" \
	-fPIC \
	-c -o core.o

# link together the core and vulkan memory allocator

echo "Linking core..."

clang++ \
	-arch arm64 \
	core.o vk_mem_alloc.o \
	$libs \
	$rpaths \
	-lassimp \
	-fPIC \
	-dynamiclib \
	-o core.dylib

# compile platform dependent
# layer as executable
# (if it isn't currently open)

if ! pgrep -x magpie_macos > /dev/null; then

	echo "Compiling platform layer..."

	clang \
		-arch arm64 \
		$opts \
		"$code/macos/macos_main.c" \
		-I"$code/" \
		$libs \
		$rpaths \
		-lSDL3 \
		-o magpie_macos
fi

cd - > /dev/null

echo "Finished!"
