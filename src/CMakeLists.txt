
# I've been developing magpie alongside a multiplayer game that I've been
# working on lately, which means that the server and client
# only link to the parts of the engine that they actually need.
# so as a result this is split:
# server = MAGPIE_BASE_SOURCES
# client = MAGPIE_BASE_SOURCES + MAGPIE_SOURCES

# todo: move physics and entity systems from game into magpie

function(magpie_warnings target)
	if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
		target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
	elseif (MSVC)
		target_compile_options(${target} PRIVATE /W4 /wd4100 /wd4201)
	endif()
endfunction()

file(GLOB MAGPIE_BASE_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/os/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/io/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/chrono/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/*.cpp
)

add_library(magpie_base STATIC ${MAGPIE_BASE_SOURCES})
target_include_directories(magpie_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(magpie_base PUBLIC magpie_ext)
magpie_warnings(magpie_base)

if (WIN32)
	target_link_libraries(magpie_base PUBLIC ws2_32)
endif()

file(GLOB_RECURSE MAGPIE_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/graphics/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/asset/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/audio/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/animation/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/script/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp
)

if (NOT MAGPIE_SOURCES)
	return()
endif()

add_library(magpie STATIC ${MAGPIE_SOURCES})
target_link_libraries(magpie PUBLIC magpie_base)

if (NOT TARGET ktx_read)
	include(FetchContent)
	set(KTX_FEATURE_TESTS OFF CACHE BOOL "" FORCE)
	set(KTX_FEATURE_TOOLS OFF CACHE BOOL "" FORCE)
	set(KTX_FEATURE_DOC OFF CACHE BOOL "" FORCE)
	set(KTX_FEATURE_JNI OFF CACHE BOOL "" FORCE)
	set(KTX_FEATURE_PY OFF CACHE BOOL "" FORCE)
	set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)
	set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
	set(KTX_FEATURE_LOADTEST_APPS OFF CACHE STRING "" FORCE)

	set(MAGPIE_SAVED_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
	set(BUILD_SHARED_LIBS OFF)
	FetchContent_Declare(ktx
			GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
			GIT_TAG v4.4.2
			GIT_SHALLOW TRUE)
	FetchContent_MakeAvailable(ktx)
	set(BUILD_SHARED_LIBS ${MAGPIE_SAVED_BUILD_SHARED_LIBS})
endif()

target_link_libraries(magpie PRIVATE ktx_read)

magpie_warnings(magpie)

if (TARGET magpie_slang)
	target_link_libraries(magpie PUBLIC magpie_slang)
	target_compile_definitions(magpie PRIVATE MAGPIE_HAS_SLANG=1)
	return()
endif()

find_library(SLANG_LIBRARY NAMES slang slang-compiler PATHS ${SLANG_ROOT}/lib ${SLANG_ROOT} NO_DEFAULT_PATH)

if (SLANG_LIBRARY AND EXISTS "${SLANG_ROOT}/include/slang/slang.h")
	target_include_directories(magpie PRIVATE ${SLANG_ROOT}/include)
	target_link_libraries(magpie PUBLIC ${SLANG_LIBRARY})
	target_compile_definitions(magpie PRIVATE MAGPIE_HAS_SLANG=1)
endif()
