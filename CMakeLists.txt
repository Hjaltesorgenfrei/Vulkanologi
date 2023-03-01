cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 23)

project(vulkanologi)

# add_compile_options(-Wall -Wextra -pedantic -Werror)



if(MSVC)
    if (NOT CMAKE_BUILD_TYPE) # Assume debug if dont have specified. Makes it work for vscode.
        set(CMAKE_BUILD_TYPE "Debug")
    endif()
    set(BUILD_OUTPUT_PATH ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
else()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build/bin)
    set(BUILD_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/bin)
endif()

function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/${SHADER})
    set(current-output-path ${BUILD_OUTPUT_PATH}/shaders/${SHADER}.spv)

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(CURRENT_OUTPUT_DIR ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${CURRENT_OUTPUT_DIR})

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
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/resources ${BUILD_OUTPUT_PATH}/resources)

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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(vulkanologi
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

add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS) # Version 145 at least
target_precompile_headers(vulkanologi PRIVATE ${Vulkan_INCLUDE_DIRS}/vulkan/vulkan.hpp ext/glm/glm/glm.hpp)
target_precompile_headers(vulkanologi PRIVATE src/BehDevice.h src/WindowWrapper.h)

target_link_libraries(vulkanologi glfw glm tinyobjloader VulkanMemoryAllocator Vulkan::Vulkan)
add_shader(vulkanologi shader.frag)
add_shader(vulkanologi shader.vert)
add_shader(vulkanologi particle.frag)
add_shader(vulkanologi particle.vert)
add_shader(vulkanologi shader_unlit.frag)
add_shader(vulkanologi shader_unlit.vert)
add_shader(vulkanologi point_light.frag)
add_shader(vulkanologi point_light.vert)
add_shader(vulkanologi particles.comp)

add_dependencies(vulkanologi copy-runtime-files)