cmake_minimum_required(VERSION 3.20)
project(renderer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_library(renderer SHARED
    renderer.cpp
    bridge.cpp
)

target_include_directories(renderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

target_link_libraries(renderer PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    "-framework Cocoa"
    "-framework IOKit"
)

set_target_properties(renderer PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../build"
    OUTPUT_NAME "renderer"
)
