project(shared)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCE_DIR}/*.cpp)

add_library(shared
    ${SOURCES}
)
target_compile_definitions(shared PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)

# TODO: I dont really want vulkan included here. It should be removed
target_link_libraries(shared glm Vulkan::Vulkan yojimbo)
target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})