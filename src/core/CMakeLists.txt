file(GLOB BASE_SRC "*.cpp" "../../3rd/imgui/*.cpp")
file(GLOB BASE_HEADERS "*.hpp" "*.h")

add_library(Core STATIC ${BASE_SRC} ${BASE_HEADERS})

target_link_libraries(Core ${Vulkan_LIBRARY} ${GLFW_DLL_DIR} ${TINY_FILE_DIALOGS_LIB_DIR})