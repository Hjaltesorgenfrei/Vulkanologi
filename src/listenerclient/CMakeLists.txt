project(SockitClient)

set(CMAKE_CXX_STANDARD 11)

add_executable(SockitClient main.cpp)
target_link_libraries(SockitClient shared yojimbo glm::glm)