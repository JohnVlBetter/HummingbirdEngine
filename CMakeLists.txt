cmake_minimum_required(VERSION 3.0)
project(HummingbirdEngine)
add_definitions(-std=c++17)

set(THIRD_PARTY_PATH "${PROJECT_SOURCE_DIR}/3rd")
#glm
set(GLM_DIR_PATH "${THIRD_PARTY_PATH}/glm")
include_directories (${GLM_DIR_PATH} src)
aux_source_directory (${GLM_DIR_PATH} GLM_SRC_LIST)

#glfw
set(GLFW_DIR_PATH "${THIRD_PARTY_PATH}/glfw")
include_directories (${GLFW_DIR_PATH}/include src)
set(GLFW_LIB_DIR_PATH "${GLFW_DIR_PATH}/lib-mingw-w64")
link_directories(${GLFW_LIB_DIR_PATH}/glfw3dll.lib)

#vulkan
set(VK_SDK_PATH "F:/Vulkan")
set(VK_SDK_INCLUDE ${VK_SDK_PATH}/Include)
set(VK_SDK_LIB ${VK_SDK_PATH}/Lib/vulkan-1.lib)
include_directories(${PROJECT_NAME} ${VK_SDK_INCLUDE})

aux_source_directory (src SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST} ${GLM_SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${VK_SDK_LIB})
target_link_libraries(${PROJECT_NAME} ${GLFW_DIR_PATH}/lib-mingw-w64/glfw3.dll)

# 复制 glfw3.dll 到编译目录下
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${GLFW_LIB_DIR_PATH}/glfw3.dll
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/
)