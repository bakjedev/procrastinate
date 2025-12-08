cmake_minimum_required (VERSION 3.28)
project("procrastinate" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_compile_definitions(VULKAN_HPP_NO_CONSTRUCTORS)

add_subdirectory(external)
add_subdirectory(engine)
add_subdirectory(runtime)

include(external/link.cmake)
