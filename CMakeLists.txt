cmake_minimum_required(VERSION 3.8)
project(magpie C CXX)

set(CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Tell volk we're compiling for Win32.
set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)

if(WIN32)

	# Compiler options.
	set(COMMON_COMPILE_OPTIONS
		/FC
		/GR-
		/EHa-
		/nologo
		/Zi
		/Od
		/W4
	)

	# Suppressed warnings.
	set(SUPPRESSED_WARNINGS
		/wd4201
		/wd4018
		/wd4996
		/wd4100
		/wd4244
	)

	# Include directories.
	include_directories(
		"${CODE_DIR}"
		"C:/VulkanSDK/1.4.328.1/Include"
		"D:/DevLibs/SDL3/include"
		"D:/DevLibs/assimp/include"
	)

	# Library directories.
	link_directories(
		"C:/VulkanSDK/1.4.328.1/Lib"
		"D:/DevLibs/SDL3/VisualC/x64/Release"
		"D:/DevLibs/assimp/lib/Release"
	)
	
elseif(APPLE)

	# Compiler options.
	set(COMMON_COMPILE_OPTIONS
		-Wall
		-Wextra
		-fno-rtti
		-fexceptions
	)

	# Suppressed warnings.
	set(SUPPRESSED_WARNINGS
		-Wno-unused-parameter
		-Wno-deprecated-declarations
		-Wno-missing-braces
		-Wno-sign-compare
		-Wno-pointer-sign
		-Wno-switch
		-Wno-missing-field-initializers
	)

	# Find required packages.
	find_package(Assimp REQUIRED)

	# Include directories.
	include_directories(game PRIVATE "${CODE_DIR}")
endif()

# Vulkan Memory Allocator Compilation.
add_library(vk_mem_alloc OBJECT "${CODE_DIR}/ext/vk_mem_alloc.cpp")
target_compile_options(vk_mem_alloc PRIVATE
	-Wno-nullability-completeness
	-Wno-missing-field-initializers
	-Wno-unused-parameter
	-Wno-unused-variable
	-Wno-unused-private-field
)

# Debug build.
set(CMAKE_BUILD_TYPE Debug)

# Game DLL.
add_library(game SHARED
	"${CODE_DIR}/app.c"
	"${CODE_DIR}/timer.c"

	"${CODE_DIR}/core/core.c"
	"${CODE_DIR}/core/core_hash.c"
	"${CODE_DIR}/core/core_math.c"
	"${CODE_DIR}/core/core_memory_arena.c"
	"${CODE_DIR}/core/core_scratch.c"
	"${CODE_DIR}/core/core_string.c"

	"${CODE_DIR}/data/hash_table.c"
	
	"${CODE_DIR}/assets/assets.c"

	"${CODE_DIR}/rendering/device.c"
	"${CODE_DIR}/rendering/bindless.c"
	"${CODE_DIR}/rendering/texture.c"
	"${CODE_DIR}/rendering/buffer.c"
	"${CODE_DIR}/rendering/pipeline.c"
	"${CODE_DIR}/rendering/command_buffer.c"
	"${CODE_DIR}/rendering/command_pool.c"
	"${CODE_DIR}/rendering/shader.c"
	"${CODE_DIR}/rendering/sync.c"
	"${CODE_DIR}/rendering/swapchain.c"
	"${CODE_DIR}/rendering/queue.c"
	"${CODE_DIR}/rendering/blend.c"

	"${CODE_DIR}/rendering/camera.c"
	"${CODE_DIR}/rendering/model.c"
	"${CODE_DIR}/rendering/render_graph.c"
	"${CODE_DIR}/rendering/shaders.c"
	"${CODE_DIR}/rendering/gbuffer.c"
	"${CODE_DIR}/rendering/render_scene.c"
	
	"${CODE_DIR}/rendering/stages/stage_clear_draw_indirect.c"
#	"${CODE_DIR}/rendering/stages/stage_compute_frustum_culling.c"
#	"${CODE_DIR}/rendering/stages/stage_deferred.c"
	"${CODE_DIR}/rendering/stages/stage_skybox.c"
	"${CODE_DIR}/rendering/stages/stage_post_processing.c"
	"${CODE_DIR}/rendering/stages/stage_blit_lighting_to_swapchain.c"
	"${CODE_DIR}/rendering/stages/stage_hdr_texture_to_cubemap.c"

	"${CODE_DIR}/ext/spirv_reflect.c"

	$<TARGET_OBJECTS:vk_mem_alloc>
)

target_compile_options(game PRIVATE ${COMMON_COMPILE_OPTIONS} ${SUPPRESSED_WARNINGS})
	
# Platform-dependent executable.
if(WIN32)
	target_link_libraries(game PRIVATE assimp-vc143-mt volk slang)
	add_executable(magpie "${CODE_DIR}/platform/win32/win32_main.c")
elseif(APPLE)
	target_link_libraries(game PRIVATE assimp::assimp volk slang)
	add_executable(magpie "${CODE_DIR}/platform/macos/macos_main.c")
endif()

target_compile_options(magpie PRIVATE ${COMMON_COMPILE_OPTIONS} ${SUPPRESSED_WARNINGS})
target_link_libraries(magpie PRIVATE game SDL3)

# Output directories.
set_target_properties(game magpie vk_mem_alloc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
