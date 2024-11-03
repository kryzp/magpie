cmake_minimum_required (VERSION 3.8)
project(lilythorn)

set(CMAKE_CXX_STANDARD 20)

set(GLM_INCLUDE_DIRS "D:/DevLibs/glm/")

set(SDL3_INCLUDE_DIRS "D:/DevLibs/SDL3/include/")
set(SDL3_LIBRARIES "D:/DevLibs/SDL3/VisualC/x64/Debug/SDL3.lib")

set(VK_INCLUDE_DIRS "D:/VulkanSDK/1.3.296.0/Include/")
set(VK_LIBRARIES "D:/VulkanSDK/1.3.296.0/Lib/vulkan-1.lib")

add_compile_definitions(LLT_DEBUG)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/common.cpp
    src/app.cpp
    src/system_backend.cpp

    src/graphics/backbuffer.cpp
    src/graphics/backend.cpp
    src/graphics/buffer.cpp
    src/graphics/buffer_mgr.cpp
    src/graphics/colour.cpp
    src/graphics/descriptor_builder.cpp
    src/graphics/descriptor_cache.cpp
    src/graphics/descriptor_pool_mgr.cpp
    src/graphics/generic_render_target.cpp
    src/graphics/render_pass_builder.cpp
    src/graphics/render_target.cpp
    src/graphics/render_target_mgr.cpp
    src/graphics/shader.cpp
    src/graphics/shader_mgr.cpp
    src/graphics/mesh.cpp
    src/graphics/sub_mesh.cpp
    src/graphics/material.cpp
    src/graphics/texture.cpp
    src/graphics/texture_mgr.cpp
    src/graphics/texture_sampler.cpp
    src/graphics/ubo_manager.cpp
    src/graphics/util.cpp
    src/graphics/image.cpp
    src/graphics/queue.cpp
    
    src/math/timer.cpp

    src/input/input.cpp
    src/input/v_key.cpp

    src/io/endian.cpp
    src/io/stream.cpp
    src/io/file_stream.cpp
    src/io/memory_stream.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${SDL3_LIBRARIES} ${VK_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${SDL3_INCLUDE_DIRS} ${VK_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS})
