
cmake_minimum_required(VERSION 3.11)
project(praas)
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

###
# Generic configuration
###
include(FetchContent)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

###
# C++ linting
### 
option(WITH_CXX_LINTING "Enable linting and formatting of C++ options." OFF)
if(${WITH_CXX_LINTING})
  find_program(CLANG_TIDY_COMMAND NAMES clang-tidy clang-tidy-14 clang-tidy-13 clang-tidy-12 clang-tidy-11 clang-tidy-10)
  if(NOT CLANG_TIDY_COMMAND)
    message(FATAL_ERROR "clang-tidy is not found!")
    set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE)
  else()
    set(CLANG_TIDY_CHECKS "*")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND};-checks=${CLANG_TIDY_CHECKS};-header-filter=.")
  endif()
endif()

###
# External dependencies
###
include(dependencies)

###
# control plane
###
file(GLOB control_plane_files "system/control-plane/*.cpp")
add_executable(control_plane ${control_plane_files})
set_target_properties(control_plane PROPERTIES RUNTIME_OUTPUT_DIRECTORY bin)
add_dependencies(control_plane cxxopts::cxxopts)
add_dependencies(control_plane sockpp-static)
add_dependencies(control_plane spdlog::spdlog)
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:cxxopts::cxxopts,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:sockpp-static,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE ${hiredis_SOURCE_DIR})
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:redis++::redis++,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE ${threadpool_SOURCE_DIR})
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:stduuid,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(control_plane PRIVATE spdlog::spdlog)
target_link_libraries(control_plane PRIVATE sockpp-static)
target_link_libraries(control_plane PRIVATE redis++::redis++_static)
target_link_libraries(control_plane PRIVATE stduuid)
target_link_libraries(control_plane PRIVATE ${HIREDIS_LIB})
#target_link_libraries(control_plane PRIVATE ${REDIS_PLUS_PLUS_LIB})

