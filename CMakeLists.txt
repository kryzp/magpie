cmake_minimum_required (VERSION 3.8)
project(magpie)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

add_executable(${PROJECT_NAME}
	src/main.cpp

	src/core/app.cpp
	src/core/common.cpp
	src/core/camera.cpp

	src/input/input.cpp

	src/io/stream.cpp
	src/io/file_stream.cpp
	src/io/memory_stream.cpp

	src/math/colour.cpp
	src/math/timer.cpp
	src/math/transform.cpp

	src/platform/platform_core.cpp

	src/graphics/bitmap.cpp
	src/graphics/pipeline.cpp
	src/graphics/vertex_format.cpp
	src/graphics/command_buffer.cpp
	src/graphics/command_pool.cpp
	src/graphics/descriptor.cpp
	src/graphics/gpu_buffer.cpp
	src/graphics/graphics_core.cpp
	src/graphics/image_view.cpp
	src/graphics/image.cpp
	src/graphics/queue.cpp
	src/graphics/render_graph.cpp
	src/graphics/sampler.cpp
	src/graphics/shader.cpp
	src/graphics/surface.cpp
	src/graphics/swapchain.cpp
	src/graphics/toolbox.cpp
	src/graphics/validation.cpp
	
	src/rendering/bindless.cpp
	src/rendering/model.cpp
	src/rendering/renderer.cpp
	src/rendering/scene.cpp
	src/rendering/shader_manager.cpp
	src/rendering/shadow_map_atlas.cpp
	src/rendering/texture_manager.cpp
	src/rendering/vertex_types.cpp
	src/rendering/model_loader.cpp

	src/third_party/volk_impl.cpp
	src/third_party/vma_impl.cpp
	src/third_party/imgui/imgui.cpp
	src/third_party/imgui/imgui_demo.cpp
	src/third_party/imgui/imgui_draw.cpp
	src/third_party/imgui/imgui_tables.cpp
	src/third_party/imgui/imgui_widgets.cpp
	src/third_party/imgui/imgui_impl_sdl3.cpp
	src/third_party/imgui/imgui_impl_vulkan.cpp
)

add_compile_definitions(MGP_DEBUG)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

if (WIN32)
	set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)

	set(SDL3_INCLUDE_DIRS "D:/DevLibs/SDL3/include/")
	set(SDL3_LIBRARIES "D:/DevLibs/SDL3/VisualC/x64/Release/SDL3.lib")

	set(VK_INCLUDE_DIRS "C:/VulkanSDK/1.4.313.1/Include/")
	set(SLANG_LIBRARIES "C:/VulkanSDK/1.4.313.1/Lib/slang.lib")

	set(ASSIMP_INCLUDE_DIRS "D:/DevLibs/assimp/include/")
	set(ASSIMP_LIBRARIES "D:/DevLibs/assimp/lib/Release/assimp-vc143-mt.lib")

	target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_LIBRARIES} ${ASSIMP_LIBRARIES} ${SLANG_LIBRARIES})
	target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_INCLUDE_DIRS} ${VK_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS})
else()
	add_compile_definitions(MGP_MAC_SUPPORT)

	find_package(SDL3 REQUIRED)
	find_package(Vulkan REQUIRED)
	find_package(glm REQUIRED)
	find_package(assimp REQUIRED)
	find_package(volk CONFIG REQUIRED)

	target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Vulkan::Vulkan glm::glm assimp::assimp volk::volk)
endif()
