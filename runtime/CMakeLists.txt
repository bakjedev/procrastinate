add_executable(${CMAKE_PROJECT_NAME})

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    src/main.cpp
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Werror
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE engine)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)