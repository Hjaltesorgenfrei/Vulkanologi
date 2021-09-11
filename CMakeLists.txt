cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 20)

project(vulkantut)

# add_compile_options(-Wall -Wextra -pedantic -Werror)

find_package(Vulkan REQUIRED)

find_package(X11 REQUIRED)
link_libraries(${X11_LIBRARIES})
add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)

find_package(glfw3 3.3 REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(vulkantut ${SOURCES})
target_link_libraries(vulkantut glfw Vulkan::Vulkan)