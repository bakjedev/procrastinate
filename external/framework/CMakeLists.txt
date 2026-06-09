# Translation of the Meson `framework` project to CMake.
# Intended to be add_subdirectory()'d into a parent CMake project.

cmake_minimum_required(VERSION 3.21)   # ~ meson_version >= 1.1.0; 3.21 gives modern Vulkan + features
project(framework VERSION 1.0 LANGUAGES CXX)

include(CheckCXXCompilerFlag)          # ~ compiler.get_supported_arguments(...)

# ---------------------------------------------------------------------------
# warning_level=3  -> base strict warnings
# werror=true      -> -Werror / /WX
# buildtype=debugoptimized -> NOT set here (== RelWithDebInfo); the *parent*
#   project controls CMAKE_BUILD_TYPE. Configure with
#   -DCMAKE_BUILD_TYPE=RelWithDebInfo for single-config generators.
# ---------------------------------------------------------------------------

# Candidate extra warnings, GCC-centric. Filtered to those the compiler
# actually accepts, mirroring get_supported_arguments(). The GCC-only ones
# (-Wduplicated-cond, -Wduplicated-branches, -Wlogical-op, -Wuseless-cast)
# are silently dropped on Clang/MSVC, exactly as Meson would drop them.
set(_candidate_warnings
  -Wshadow
  -Wnon-virtual-dtor
  -Wold-style-cast
  -Wcast-align
  -Wunused
  -Woverloaded-virtual
  -Wconversion
  -Wsign-conversion
  -Wnull-dereference
  -Wdouble-promotion
  -Wformat=2
  -Wimplicit-fallthrough
  -Wmisleading-indentation
  -Wduplicated-cond
  -Wduplicated-branches
  -Wlogical-op
  -Wuseless-cast
  -Wsuggest-override
)

# Make Clang actually reject unknown -W flags during the probe (otherwise it
# accepts them and only complains at real compile time).
set(CMAKE_REQUIRED_FLAGS_SAVED "${CMAKE_REQUIRED_FLAGS}")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_REQUIRED_FLAGS "-Werror=unknown-warning-option")
endif()

set(framework_warnings "")
foreach(flag IN LISTS _candidate_warnings)
  string(MAKE_C_IDENTIFIER "HAVE_CXX_${flag}" _flag_var)
  check_cxx_compiler_flag("${flag}" ${_flag_var})
  if(${_flag_var})
    list(APPEND framework_warnings "${flag}")
  endif()
endforeach()

set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS_SAVED}")

# Base level + werror.
if(MSVC)
  list(APPEND framework_warnings /W4 /WX)
else()
  list(APPEND framework_warnings -Wall -Wextra -Wpedantic -Werror)
endif()

# ---------------------------------------------------------------------------
# dependency('vulkan', include_type: 'system')
# Imported-target include dirs are treated as SYSTEM by CMake by default,
# so Vulkan-header warnings are already suppressed -- matches include_type:system.
# ---------------------------------------------------------------------------
find_package(Vulkan REQUIRED)

# ---------------------------------------------------------------------------
# library('framework-lib', ...)  +  declare_dependency(...)
# In CMake these collapse into one target: PUBLIC props on the target are the
# declare_dependency, PRIVATE props are the library's own build flags.
# Library type left unspecified -> follows BUILD_SHARED_LIBS, like Meson's
# default_library option (Meson defaults to shared).
# ---------------------------------------------------------------------------
add_library(framework-lib
  src/context.cpp
  src/graph.cpp
  src/pass_builder.cpp
)

# include_directories('src') exposed to consumers (declare_dependency)  -> PUBLIC
target_include_directories(framework-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# cpp_std=c++20. PUBLIC, not PRIVATE: src/ headers are exported, so a consumer
# including them needs C++20. (Strictly, Meson's cpp_std does not propagate via
# declare_dependency; PRIVATE would be the literal translation. PUBLIC is the
# correct practical choice here -- change to PRIVATE if the headers are C++17-clean.)
target_compile_features(framework-lib PUBLIC cxx_std_20)

# add_project_arguments(warnings) does NOT propagate through declare_dependency,
# so consumers must not inherit -Werror/strict warnings -> PRIVATE.
target_compile_options(framework-lib PRIVATE ${framework_warnings})

# dependencies : [vulkan_dep] on both the library and declare_dependency -> PUBLIC
target_link_libraries(framework-lib PUBLIC Vulkan::Vulkan)

# Clean consumable name (the declare_dependency equivalent for parents):
#   target_link_libraries(my_app PRIVATE framework::framework)
add_library(framework::framework ALIAS framework-lib)
