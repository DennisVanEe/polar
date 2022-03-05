cmake_minimum_required (VERSION 3.16)
project(polar VERSION 1.0.0 LANGUAGES C CXX)

add_executable(polar
    "src/main.cpp"
    "src/context.hpp"
    "src/context.cpp"
    "src/gpu_allocator.hpp"
    "src/gpu_allocator.cpp"
    "src/util.hpp"
    "src/util.cpp"

    # External Library:
    "extern/vma-2.3.0/vk_mem_alloc.h"
    "extern/vma-2.3.0/vk_mem_alloc.cpp"
    "extern/tinygltf/tiny_gltf.h"
    "extern/tinygltf/tiny_gltf.cpp"
)

target_include_directories(polar PRIVATE "src/")

set_target_properties(polar
    PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

#
# External Libraries:
#

add_subdirectory("extern/glm-0.9.9.8")
add_subdirectory("extern/spdlog-1.9.2")
add_subdirectory("extern/glfw-3.3.6")

target_link_libraries(polar PRIVATE glfw glm spdlog)

# Include for any external libraries:
target_include_directories(polar PRIVATE "extern/vma-2.3.0/")
target_include_directories(polar PRIVATE "extern/tinygltf/")

find_package(Vulkan 1.2.162 REQUIRED)
target_include_directories(polar PUBLIC ${Vulkan_INCLUDE_DIRS})

#
# Compiler Definitions:
#

target_compile_definitions(polar PRIVATE
    GLFW_INCLUDE_VULKAN
    VULKAN_HPP_NO_CONSTRUCTORS
    VMA_STATIC_VULKAN_FUNCTIONS=0     # We don't want to statically link against vulkan (we use dynamic for everything)
    VMA_DYNAMIC_VULKAN_FUNCTIONS=0    # We want to manually let VMA fail an assertion if I'm missing a function
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

#
# Configuration File:
#

configure_file("configure.hpp" "${PROJECT_BINARY_DIR}/include/configure.hpp")
target_include_directories(polar PRIVATE "${PROJECT_BINARY_DIR}/include/")
