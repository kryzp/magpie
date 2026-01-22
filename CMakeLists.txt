cmake_minimum_required(VERSION 3.10)
project(magpie CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

set(CODE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Compiler Options.
set(COMPILE_OPTIONS
	-fno-rtti
	-fexceptions
	-Wno-unused-parameter
	-Wno-deprecated-declarations
	-Wno-missing-braces
	-Wno-sign-compare
	-Wno-pointer-sign
	-Wno-switch
	-Wno-missing-field-initializers
	-Wno-nullability-completeness
	-Wno-missing-field-initializers
	-Wno-unused-parameter
	-Wno-unused-variable
	-Wno-unused-private-field
	-Wno-switch
	-Wno-c++98-compat
	-Wno-c++98-compat-pedantic
)

if(WIN32)

	# Tell volk we're compiling for Win32.
	set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)

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

	# Tell volk we're compiling for macOS.
	set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_MACOS_MVK)

	# Find required packages.
	find_package(Assimp REQUIRED)
	find_package(Vulkan REQUIRED)
	find_package(SDL3 REQUIRED)

endif()

# Program.
add_executable(magpie
	"${CODE_DIR}/platform/win32/win32_main.cpp"

	"${CODE_DIR}/app.cpp"

	"${CODE_DIR}/core/hash.cpp"
	"${CODE_DIR}/core/memory_arena.cpp"
	"${CODE_DIR}/core/scratch.cpp"

	"${CODE_DIR}/math/timer.cpp"
	"${CODE_DIR}/math/matrix.cpp"
	"${CODE_DIR}/math/vec2.cpp"
	"${CODE_DIR}/math/vec3.cpp"

	"${CODE_DIR}/io/filesystem.cpp"

	"${CODE_DIR}/assets/assets.cpp"
	"${CODE_DIR}/assets/texture_serializer.cpp"
	"${CODE_DIR}/assets/shader_serializer.cpp"
	"${CODE_DIR}/assets/model_serializer.cpp"

	"${CODE_DIR}/job/job.cpp"
	
	"${CODE_DIR}/dev/cpu_profiler.cpp"

	"${CODE_DIR}/graphics/bindless.cpp"
	"${CODE_DIR}/graphics/camera.cpp"
	"${CODE_DIR}/graphics/command_buffer.cpp"
	"${CODE_DIR}/graphics/command_pool.cpp"
	"${CODE_DIR}/graphics/device.cpp"
	"${CODE_DIR}/graphics/gpu_buffer.cpp"
	"${CODE_DIR}/graphics/queue.cpp"
	"${CODE_DIR}/graphics/render_graph.cpp"
	"${CODE_DIR}/graphics/render_scene.cpp"
	"${CODE_DIR}/graphics/sync.cpp"
	"${CODE_DIR}/graphics/model.cpp"

	"${CODE_DIR}/graphics/renderers/skybox_renderer.cpp"
	"${CODE_DIR}/graphics/renderers/post_processing.cpp"

	"${CODE_DIR}/ext/vk_mem_alloc.cpp"
	"${CODE_DIR}/ext/spirv_reflect.cpp"
	"${CODE_DIR}/ext/volk.cpp"
)

target_compile_definitions(magpie PUBLIC SPIRV_REFLECT_USE_SYSTEM_SPIRV_H)
target_include_directories(magpie PRIVATE "${CODE_DIR}" "${Vulkan_INCLUDE_DIRS}")

if(WIN32)
	target_link_libraries(magpie assimp-vc143-mt volk slang SDL3)
	set_property(TARGET magpie PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
elseif(APPLE)
	target_link_libraries(magpie SDL3::SDL3 Vulkan::Vulkan assimp::assimp slang)
endif()

target_compile_options(magpie PRIVATE ${COMPILE_OPTIONS})

# Output directories.
set_target_properties(magpie PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
