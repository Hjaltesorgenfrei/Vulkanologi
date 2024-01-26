project(shared)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCE_DIR}/*.cpp)

add_library(shared
    ${SOURCES}
)

target_link_libraries(shared yojimbo)
target_include_directories(shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})