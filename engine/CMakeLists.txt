include(../external/shaders.cmake)

compile_shaders(
        Shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders
)

add_library(engine STATIC)

add_dependencies(engine Shaders)

target_sources(engine PRIVATE
        src/core/engine.cpp
        src/core/events.cpp
        src/core/window.cpp
        src/core/imgui.cpp
        src/input/input.cpp
        src/ecs/scene.cpp
        src/resource/types/mesh_resource.cpp
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
        src/render/vk_barriers.cpp
)

target_compile_definitions(engine PRIVATE NOMINMAX)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(engine PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -Wshadow # warn the user if a variable declaration shadows one from a parent context
            -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
            -Wold-style-cast # warn for c-style casts
            -Wcast-align # warn for potential performance problem casts
            -Wunused # warn on anything being unused
            -Woverloaded-virtual # warn if you overload (not override) a virtual function
            -Wconversion # warn on type conversions that may lose data
            -Wsign-conversion # warn on sign conversions
            -Wnull-dereference # warn if a null dereference is detected
            -Wdouble-promotion # warn if float is implicit promoted to double
            -Wformat=2 # warn on security issues around functions that format output (ie printf)
            -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
            -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
    )
endif ()

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
