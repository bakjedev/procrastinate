add_library(engine STATIC)

target_sources(engine
  PRIVATE
    src/core/engine.cpp
)

target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
