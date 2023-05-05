SET(EXAMPLE_NAME "HummingbirdEngine")

file(GLOB SHADERS "../../artRes/shaders/*.vert" "../../artRes/shaders/*.frag" "../../artRes/shaders/*.comp" "../../artRes/shaders/*.geom" "../../artRes/shaders/*.tesc" "../../artRes/shaders/*.tese")
source_group("Shaders" FILES ${SHADERS})

file(GLOB SOURCE *.cpp ${BASE_HEADERS})

add_executable(${EXAMPLE_NAME} ${SOURCE} ${SHADERS})

target_link_libraries(${EXAMPLE_NAME} Core ${Vulkan_LIBRARY} ${GLFW_DLL_DIR})