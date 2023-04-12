if(MSVC)
    if (NOT CMAKE_BUILD_TYPE) # Assume debug if dont have specified. Makes it work for vscode.
        set(CMAKE_BUILD_TYPE "Debug")
    endif()
    set(BUILD_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
else()
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/lib)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build/bin)
    set(BUILD_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/build/bin)
endif()

function(add_shader TARGET SHADER)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER})
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
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${BUILD_OUTPUT_PATH}/resources)

find_package(Vulkan REQUIRED)

set(SOURCE_DIR engine)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCE_DIR}/*.cpp)

add_executable(vulkanologi
    ${SOURCES}
)

if (NOT WIN32)
    find_package(X11 REQUIRED)
    target_link_libraries(vulkanologi ${X11_LIBRARIES})
    target_compile_definitions(vulkanologi PRIVATE -DVK_USE_PLATFORM_XLIB_KHR)
endif()

add_compile_definitions(VULKAN_HPP_NO_STRUCT_CONSTRUCTORS) # Version 145 at least
target_precompile_headers(vulkanologi PRIVATE ${Vulkan_INCLUDE_DIRS}/vulkan/vulkan.hpp "${GLM_INCLUDE_DIR}/glm/glm.hpp")
target_precompile_headers(vulkanologi PRIVATE ${SOURCE_DIR}/BehDevice.hpp ${SOURCE_DIR}/WindowWrapper.hpp)

target_link_libraries(vulkanologi glfw glm tinyobjloader VulkanMemoryAllocator Vulkan::Vulkan ktx imgui stb EnTT Jolt)

add_shader(vulkanologi shader.frag)
add_shader(vulkanologi shader.vert)
add_shader(vulkanologi particle.frag)
add_shader(vulkanologi particle.vert)
add_shader(vulkanologi shader_unlit.frag)
add_shader(vulkanologi shader_unlit.vert)
add_shader(vulkanologi shader_unlit_line.frag)
add_shader(vulkanologi shader_unlit_line.vert)
add_shader(vulkanologi point_light.frag)
add_shader(vulkanologi point_light.vert)
add_shader(vulkanologi particles.comp)

add_dependencies(vulkanologi copy-runtime-files)