project(renderer)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SOURCE_DIR}/*.cpp)

add_library(renderer
    ${SOURCES}
)

if(NOT WIN32)
    find_package(X11 REQUIRED)
    target_link_libraries(renderer ${X11_LIBRARIES})
    target_compile_definitions(renderer PRIVATE -DVK_USE_PLATFORM_XLIB_KHR)
endif()

target_compile_definitions(renderer PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)
target_precompile_headers(renderer PRIVATE ${Vulkan_INCLUDE_DIRS}/vulkan/vulkan.hpp "${GLM_INCLUDE_DIR}/glm/glm.hpp")
target_precompile_headers(renderer PRIVATE ${SOURCE_DIR}/BehDevice.hpp ${SOURCE_DIR}/WindowWrapper.hpp)

target_link_libraries(renderer glfw glm tinyobjloader VulkanMemoryAllocator Vulkan::Vulkan ktx imgui stb shared)

target_include_directories(renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
