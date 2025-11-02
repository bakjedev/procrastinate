cmake_minimum_required (VERSION 3.28)
project("procrastinate" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	
else()
	add_compile_options(
		-Wall
		-Wextra
		-Wpedantic
		-Werror
	)
endif()

add_subdirectory(engine)
add_subdirectory(runtime)