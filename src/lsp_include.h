
// remove warnings from external libraries
#pragma warning(push)
#pragma warning(disable:4310 4709 4701 4702 4324)

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb/stb_image_write.h"

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <vma/vk_mem_alloc.h>

#include "ext/slang/slang_compiler.h"

#define SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
#include "ext/spirv/spirv_reflect.h"
#include "ext/spirv/spirv_reflect.c"

#define MINIAUDIO_IMPLEMENTATION
#include "ext/ma/miniaudio.h"

#define CGLTF_IMPLEMENTATION
#include "ext/gltf/cgltf.h"

#define MAKE_LIB
#include "ext/lua/onelua.c"

#pragma warning(pop)

// who decided to name these macros ffs
#undef min
#undef max
#undef near
#undef far

// for LSP's

#include "core/core_inc.h"
#include "os/os_inc.h"
#include "io/io_inc.h"
#include "chrono/chrono_inc.h"
#include "script/script_inc.h"
#include "graphics/graphics_inc.h"
#include "audio/audio_inc.h"
#include "asset/asset_inc.h"
#include "animation/animation_inc.h"
#include "render/render_inc.h"
#include "physics/physics_inc.h"
#include "entity/entity_inc.h"
#include "dev/dev_inc.h"
#include "game/game_inc.h"
#include "app.h"
