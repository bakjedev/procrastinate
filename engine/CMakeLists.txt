add_library(engine STATIC)

target_sources(engine PRIVATE
	src/core/engine.cpp
	src/core/events.cpp
	src/core/window.cpp
)

target_compile_options(engine PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Werror
)

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
