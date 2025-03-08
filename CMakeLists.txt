cmake_minimum_required (VERSION 3.8)
project(lilythorn)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/core/common.cpp
    src/core/app.cpp
    src/core/platform.cpp

    src/rendering/renderer.cpp
    src/rendering/gpu_buffer_mgr.cpp
    src/rendering/render_target_mgr.cpp
    src/rendering/shader_mgr.cpp
    src/rendering/shader_buffer_mgr.cpp
    src/rendering/sub_mesh.cpp
    src/rendering/mesh.cpp
    src/rendering/mesh_loader.cpp
    src/rendering/material.cpp
	src/rendering/material_system.cpp
    src/rendering/texture_mgr.cpp
    src/rendering/image.cpp
    src/rendering/camera.cpp
    src/rendering/render_object.cpp
    src/rendering/gpu_particles.cpp
    src/rendering/scene.cpp

    src/rendering/passes/forward_pass.cpp
    src/rendering/passes/post_process_pass.cpp

    src/vulkan/backbuffer.cpp
    src/vulkan/backend.cpp
    src/vulkan/command_buffer.cpp
    src/vulkan/descriptor_builder.cpp
    src/vulkan/descriptor_allocator.cpp
    src/vulkan/descriptor_layout_cache.cpp
    src/vulkan/gpu_buffer.cpp
    src/vulkan/generic_render_target.cpp
	src/vulkan/render_info.cpp
    src/vulkan/render_target.cpp
    src/vulkan/shader.cpp
    src/vulkan/shader_buffer.cpp
    src/vulkan/pipeline.cpp
    src/vulkan/texture.cpp
    src/vulkan/texture_sampler.cpp
    src/vulkan/util.cpp
    src/vulkan/queue.cpp
    src/vulkan/vertex_format.cpp

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

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

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

	target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
	target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan glm::glm assimp::assimp)
endif()
