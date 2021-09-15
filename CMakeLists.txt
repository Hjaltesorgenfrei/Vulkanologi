cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 20)

project(vulkantut)

# add_compile_options(-Wall -Wextra -pedantic -Werror)

function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/${SHADER})
    set(current-output-path ${CMAKE_BINARY_DIR}/shaders/${SHADER}.spv)

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

find_package(Vulkan REQUIRED)

find_package(X11 REQUIRED)
link_libraries(${X11_LIBRARIES})
add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)

find_package(glfw3 3.3 REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(vulkantut ${SOURCES})
target_link_libraries(vulkantut glfw Vulkan::Vulkan)
add_shader(vulkantut shader.frag)
add_shader(vulkantut shader.vert)