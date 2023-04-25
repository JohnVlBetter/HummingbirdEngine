file(GLOB BASE_SRC "*.cpp" "../external/imgui/*.cpp")
file(GLOB BASE_HEADERS "*.hpp" "*.h")

add_library(base STATIC ${BASE_SRC} ${BASE_HEADERS})

target_link_libraries(base ${Vulkan_LIBRARY} ${WINLIBS} ${DRACO_DECODER_LIBRARY})