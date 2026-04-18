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
		-I"/Users/kryzp/VulkanSDK/1.4.335.1/macOS/include" \
		-w \
		-fPIC \
		-std=c++20 \
		-c -o vk_mem_alloc.o
fi

# compile platform agnostic program

echo "Compiling app..."

clang \
	-arch arm64 \
	$opts "$code/app.c" \
	-I"$code/" \
	-I"/Users/kryzp/VulkanSDK/1.4.335.1/macOS/include" \
    -I"/opt/homebrew/include" \
    -L"/opt/homebrew/lib" \
    -lassimp \
	-fdeclspec \
	-Wno-switch \
	-Wno-ignored-attributes \
	-fPIC \
	-c -o app.o

# link together the core and vulkan memory allocator

echo "Linking app..."

clang++ \
	-arch arm64 \
	app.o vk_mem_alloc.o \
	$libs \
	$rpaths \
	-lassimp \
	-fPIC \
	-dynamiclib \
	-o app.dylib

# compile platform dependent
# layer as executable
# (if it isn't currently open)

if ! pgrep -x magpie_macos > /dev/null; then

	echo "Compiling platform layer..."

	clang \
		-arch arm64 \
		$opts \
		"$code/os/win32/win32_main.c" \
		-I"$code/" \
		$libs \
		$rpaths \
		-lSDL3 \
		-o magpie_win32
fi

cd - > /dev/null

echo "Finished!"
