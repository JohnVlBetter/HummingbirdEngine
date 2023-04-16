cmake_minimum_required(VERSION 3.2)
project(HummingbirdEngine)
project(${PROJECT_NAME} VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THIRD_PARTY_PATH "${PROJECT_SOURCE_DIR}/3rd")

#glm
set(GLM_DIR_PATH "${THIRD_PARTY_PATH}/glm")
include_directories (${GLM_DIR_PATH})
aux_source_directory (${GLM_DIR_PATH} GLM_SRC_LIST)

#stb_image
set(STB_IMAGE_DIR_PATH "${THIRD_PARTY_PATH}/stb_image")
include_directories (${STB_IMAGE_DIR_PATH})

#vulkan
set(VK_SDK_PATH "F:/Vulkan")
set(VK_SDK_INCLUDE ${VK_SDK_PATH}/Include)
include_directories(${VK_SDK_INCLUDE}) 
FIND_LIBRARY(VULKAN_1_LIB vulkan-1 ${VK_SDK_PATH}/Lib)
message("${VULKAN_1_LIB}")

#glfw
set(GLFW_DIR_PATH "${THIRD_PARTY_PATH}/glfw")
include_directories (${GLFW_DIR_PATH}/include)
FIND_LIBRARY(GLFW_LIB glfw3dll.lib ${GLFW_DIR_PATH}/lib-mingw-w64)
message("${GLFW_LIB}")
#set(GLFW_LIB_DIR_PATH "${GLFW_DIR_PATH}/lib-mingw-w64")

aux_source_directory (src/scripts SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST} ${GLM_SRC_LIST})
#link_directories(${GLFW_LIB} ${VULKAN_1_LIB})
target_link_libraries(${PROJECT_NAME} PRIVATE ${GLFW_DIR_PATH}/lib-mingw-w64/glfw3.dll)
target_link_libraries(${PROJECT_NAME} PRIVATE ${VULKAN_1_LIB})

#target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_DIR_PATH}/include)
#target_include_directories(${PROJECT_NAME} PRIVATE ${VK_SDK_INCLUDE})

# 复制 glfw3.dll 到编译目录下
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${GLFW_DIR_PATH}/lib-mingw-w64/glfw3.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/
)