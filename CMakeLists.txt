cmake_minimum_required(VERSION 3.2)

set(NAME HummingbirdEngine)
project(${NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

#3rd
include_directories(3rd)
include_directories(3rd/glm)
include_directories(3rd/gli)
include_directories(3rd/imgui)
include_directories(3rd/tinygltf)

#core
include_directories(src/core)

#vulkan
find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/3rd/vulkan/lib)
IF (Vulkan_LIBRARY)
	set(Vulkan_FOUND ON)
	MESSAGE("Using bundled Vulkan library version")
ENDIF()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

IF(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF(MSVC)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

file(GLOB SOURCE *.cpp)

add_subdirectory(src/core)
add_subdirectory(src/example)