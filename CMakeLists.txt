cmake_minimum_required (VERSION 3.8)
project(magpie)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/core/common.cpp
    src/core/app.cpp
    src/core/platform.cpp
    src/core/debug_ui.cpp

    src/rendering/camera.cpp
    src/rendering/scene.cpp
    src/rendering/model.cpp
    src/rendering/model_loader.cpp

    src/vulkan/core.cpp
    src/vulkan/toolbox.cpp
    src/vulkan/swapchain.cpp
    src/vulkan/descriptor.cpp
    src/vulkan/vertex_format.cpp
    src/vulkan/validation.cpp
    src/vulkan/pipeline_cache.cpp
    src/vulkan/render_info.cpp
    src/vulkan/image.cpp
    src/vulkan/image_view.cpp
    src/vulkan/sampler.cpp
    src/vulkan/surface.cpp
    src/vulkan/command_buffer.cpp
    src/vulkan/command_pool.cpp
    src/vulkan/gpu_buffer.cpp
    src/vulkan/shader.cpp
    src/vulkan/sync.cpp
    src/vulkan/queue.cpp
    src/vulkan/bitmap.cpp
    src/vulkan/bindless.cpp

	src/math/colour.cpp
    src/math/timer.cpp
    src/math/transform.cpp

    src/input/input.cpp

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

add_compile_definitions(MGP_DEBUG)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

if(WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)

	set(GLM_INCLUDE_DIRS "D:/DevLibs/glm/")

	set(SDL3_INCLUDE_DIRS "D:/DevLibs/SDL3/include/")
	set(SDL3_LIBRARIES "D:/DevLibs/SDL3/VisualC/x64/Release/SDL3.lib")

	set(VK_INCLUDE_DIRS "D:/VulkanSDK/1.3.296.0/Include/")

	set(ASSIMP_INCLUDE_DIRS "D:/DevLibs/assimp/include/")
	set(ASSIMP_LIBRARIES "D:/DevLibs/assimp/lib/Release/assimp-vc143-mt.lib")

	target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL3_LIBRARIES} ${ASSIMP_LIBRARIES})

	target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_INCLUDE_DIRS} ${VK_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${ASSIMP_INCLUDE_DIRS})
else()
	add_compile_definitions(MGP_MAC_SUPPORT)

	find_package(SDL3 REQUIRED)
	find_package(Vulkan REQUIRED)
	find_package(glm REQUIRED)
	find_package(assimp REQUIRED)

	target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 Vulkan::Vulkan glm::glm assimp::assimp)
endif()
