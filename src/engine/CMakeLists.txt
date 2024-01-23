project(engine)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCE_DIR}/*.cpp)

add_library(engine
    ${SOURCES}
)

target_link_libraries(engine renderer EnTT Jolt yojimbo)
target_precompile_headers(engine PRIVATE ${SOURCE_DIR}/Components.hpp)

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
