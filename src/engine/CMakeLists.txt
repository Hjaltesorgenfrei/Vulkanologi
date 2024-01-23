project(engine)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCE_DIR}/*.cpp)

add_library(engine
    ${SOURCES}
)

target_link_libraries(engine renderer EnTT Jolt yojimbo)
target_precompile_headers(engine PUBLIC ${CPM_PACKAGE_jolt_SOURCE_DIR}/Jolt/Jolt.h ${CPM_PACKAGE_entt_SOURCE_DIR}/src/entt/entt.hpp)
target_precompile_headers(engine PUBLIC ${SOURCE_DIR}/Components.hpp)

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
