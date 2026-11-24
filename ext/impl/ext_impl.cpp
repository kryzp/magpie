// Single-header library implementations used by magpie.
// (volk, SPIRV-Reflect, Lua and ImGui are compiled from their own sources, see ext/CMakeLists.txt)

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#define CGLTF_IMPLEMENTATION
#include <gltf/cgltf.h>
