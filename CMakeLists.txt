cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 20)

project(vulkantut)

# add_compile_options(-Wall -Wextra -pedantic -Werror)

if(MSVC)
    set(build-output-path ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE_INIT})
else()
    set(build-output-path ${CMAKE_BINARY_DIR})
endif()

function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/${SHADER})
    set(current-output-path ${build-output-path}/shaders/${SHADER}.spv)

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
           OUTPUT ${current-output-path}
           COMMAND ${GLSLC} -o ${current-output-path} ${current-shader-path}
           DEPENDS ${current-shader-path}
           IMPLICIT_DEPENDS CXX ${current-shader-path}
           VERBATIM)

    # Make sure our build depends on this output.
    set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

add_custom_target(copy-runtime-files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/resources ${build-output-path}/resources)

find_package(Vulkan REQUIRED)

if (NOT WIN32)
    find_package(X11 REQUIRED)
    link_libraries(${X11_LIBRARIES})
    add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)
endif()

# Build GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only")
add_subdirectory(ext/glfw)

# GLM
add_subdirectory(ext/glm)

# STB
include_directories(ext/stb)

# tinyobjloader
add_subdirectory(ext/tinyobjloader)

# VMA
add_subdirectory(ext/VulkanMemoryAllocator)

# Imgui
include_directories(ext/imgui)

include_directories(ext/ImGuizmo)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(vulkantut 
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/ImGuizmo/ImGuizmo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/ImGuizmo/ImGuizmo.cpp
)
target_link_libraries(vulkantut glfw glm tinyobjloader VulkanMemoryAllocator Vulkan::Vulkan)
add_shader(vulkantut shader.frag)
add_shader(vulkantut shader.vert)

add_dependencies(vulkantut copy-runtime-files)