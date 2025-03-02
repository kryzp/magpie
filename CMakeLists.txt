cmake_minimum_required (VERSION 3.8)
project(lilythorn)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/common.cpp
    src/app.cpp
    src/platform.cpp
    src/renderer.cpp
    src/camera.cpp
    src/render_object.cpp
    src/gpu_particles.cpp

    src/graphics/backbuffer.cpp
    src/graphics/backend.cpp
    src/graphics/command_buffer.cpp
    src/graphics/descriptor_builder.cpp
    src/graphics/descriptor_allocator.cpp
    src/graphics/descriptor_layout_cache.cpp
    src/graphics/gpu_buffer.cpp
    src/graphics/gpu_buffer_mgr.cpp
    src/graphics/generic_render_target.cpp
	src/graphics/render_info.cpp
    src/graphics/render_target.cpp
    src/graphics/render_target_mgr.cpp
    src/graphics/shader.cpp
    src/graphics/shader_mgr.cpp
    src/graphics/shader_buffer.cpp
    src/graphics/shader_buffer_mgr.cpp
    src/graphics/sub_mesh.cpp
    src/graphics/pipeline.cpp
    src/graphics/mesh.cpp
    src/graphics/mesh_loader.cpp
    src/graphics/material.cpp
	src/graphics/material_system.cpp
    src/graphics/texture.cpp
    src/graphics/texture_mgr.cpp
    src/graphics/texture_sampler.cpp
    src/graphics/util.cpp
    src/graphics/image.cpp
    src/graphics/queue.cpp
    src/graphics/vertex_format.cpp

	src/math/colour.cpp
    src/math/timer.cpp
    src/math/transform.cpp

    src/input/input.cpp
    src/input/v_key.cpp

    src/io/stream.cpp
    src/io/file_stream.cpp
    src/io/memory_stream.cpp

    src/third_party/vk_mem_alloc.cpp
    src/third_party/volk_impl.cpp

    src/third_party/imgui/imgui.cpp
    src/third_party/imgui/imgui_demo.cpp
    src/third_party/imgui/imgui_draw.cpp
    src/third_party/imgui/imgui_tables.cpp
    src/third_party/imgui/imgui_widgets.cpp
    src/third_party/imgui/imgui_impl_sdl3.cpp
    src/third_party/imgui/imgui_impl_vulkan.cpp
)

set(DEBUG_MODE true CACHE BOOL "Enable debug mode")

if(DEBUG_MODE)
	add_compile_definitions(LLT_DEBUG)
endif()

if(WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)

	set(GLM_INCLUDE_DIRS "D:/DevLibs/glm/")

	set(SDL3_INCLUDE_DIRS "D:/DevLibs/SDL3/include/")
	set(SDL3_LIBRARIES "D:/DevLibs/SDL3/VisualC/x64/Release/SDL3.lib")

	set(VK_INCLUDE_DIRS "D:/VulkanSDK/1.3.296.0/Include/")

	set(ASSIMP_INCLUDE_DIRS "D:/DevLibs/assimp/include/")
	set(ASSIMP_LIBRARIES "D:/DevLibs/assimp/lib/Release/assimp-vc143-mt.lib")

	target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL3_LIBRARIES} ${ASSIMP_LIBRARIES})
	target_include_directories(${PROJECT_NAME} PUBLIC ${SDL3_INCLUDE_DIRS} ${VK_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS})
else()
	add_compile_definitions(LLT_MAC_SUPPORT)

	add_compile_definitions(RVN_USE_SDL3)
	add_compile_definitions(RVN_USE_VULKAN)

	find_package(SDL3 REQUIRED)
	find_package(Vulkan REQUIRED)
	find_package(glm REQUIRED)
	find_package(assimp REQUIRED)

	target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3 Vulkan::Vulkan glm::glm assimp::assimp)
endif()
