project(Server)

set(CMAKE_CXX_STANDARD 11)

add_executable(Server main.cpp)
target_link_libraries(Server shared yojimbo glm::glm)