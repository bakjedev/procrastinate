include(../external/shaders.cmake)

add_library(engine STATIC)

target_sources(engine PRIVATE
	src/core/engine.cpp
	src/core/events.cpp
	src/core/window.cpp
	src/input/input.cpp
	src/render/vk_renderer.cpp
	src/render/vk_command_pool.cpp
	src/render/vk_frame.cpp
	src/render/vk_instance.cpp
	src/render/vk_device.cpp
	src/render/vk_surface.cpp
	src/render/vk_swap_chain.cpp
	src/render/vk_buffer.cpp
	src/render/vk_image.cpp
	src/render/vk_shader.cpp
	src/render/vk_descriptor.cpp
	src/render/vk_pipeline.cpp
)

target_compile_options(engine PRIVATE
	-Wall
	-Wextra
	-Wpedantic
	-Werror
)

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

compile_shaders(
	${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders
	${CMAKE_BINARY_DIR}/assets/shaders
)