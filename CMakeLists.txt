cmake_minimum_required (VERSION 3.28)
project("procrastinate" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external)
add_subdirectory(engine)
add_subdirectory(runtime)

include(external/link.cmake)