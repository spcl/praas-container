
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
# Runtime library.
###
file(GLOB runtime_library_files "runtime/lib/*.cpp")
add_library(runtime_library STATIC ${runtime_library_files})
set_target_properties(runtime_library PROPERTIES RUNTIME_OUTPUT_DIRECTORY lib)
add_dependencies(runtime_library cxxopts::cxxopts)
add_dependencies(runtime_library sockpp-static)
add_dependencies(runtime_library spdlog::spdlog)
add_dependencies(runtime_library tcpunch)
target_include_directories(runtime_library PUBLIC "runtime/include")
target_include_directories(runtime_library SYSTEM PRIVATE $<TARGET_PROPERTY:cxxopts::cxxopts,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PRIVATE $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PRIVATE $<TARGET_PROPERTY:sockpp-static,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PRIVATE $<TARGET_PROPERTY:tcpunch,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PUBLIC ${threadpool_SOURCE_DIR})
target_link_libraries(runtime_library PRIVATE spdlog::spdlog)
target_link_libraries(runtime_library PRIVATE sockpp-static)
target_link_libraries(runtime_library PRIVATE tcpunch)

###
# Session exec
###
file(GLOB runtime_session_files "runtime/bin/*.cpp")
add_executable(runtime_session ${runtime_session_files})
set_target_properties(runtime_session PROPERTIES RUNTIME_OUTPUT_DIRECTORY bin)
add_dependencies(runtime_session cxxopts::cxxopts)
add_dependencies(runtime_session sockpp-static)
add_dependencies(runtime_session spdlog::spdlog)
add_dependencies(runtime_session runtime_library)
add_dependencies(runtime_session tcpunch)
target_include_directories(runtime_session SYSTEM PRIVATE $<TARGET_PROPERTY:cxxopts::cxxopts,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_session SYSTEM PRIVATE $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_session SYSTEM PRIVATE $<TARGET_PROPERTY:sockpp-static,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_session SYSTEM PRIVATE $<TARGET_PROPERTY:runtime_library,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_session SYSTEM PRIVATE $<TARGET_PROPERTY:tcpunch,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(runtime_session PRIVATE runtime_library)
target_link_libraries(runtime_session PRIVATE spdlog::spdlog)
target_link_libraries(runtime_session PRIVATE sockpp-static)
target_link_libraries(runtime_session PRIVATE tcpunch)

###
# Common system library.
###
file(GLOB common_library_files "system/common/*.cpp")
add_library(common_library STATIC ${common_library_files})
set_target_properties(common_library PROPERTIES RUNTIME_OUTPUT_DIRECTORY lib)

###
# control plane
###
file(GLOB control_plane_files "system/control-plane/*.cpp")
add_executable(control_plane ${control_plane_files})
set_target_properties(control_plane PROPERTIES RUNTIME_OUTPUT_DIRECTORY bin)
add_dependencies(control_plane cxxopts::cxxopts)
add_dependencies(control_plane sockpp-static)
add_dependencies(control_plane spdlog::spdlog)
add_dependencies(control_plane common_library)
add_dependencies(control_plane Crow::Crow)
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:cxxopts::cxxopts,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:sockpp-static,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE ${hiredis_SOURCE_DIR})
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:redis++::redis++,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE ${threadpool_SOURCE_DIR})
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:stduuid,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(control_plane SYSTEM PRIVATE $<TARGET_PROPERTY:Crow::Crow,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(control_plane PRIVATE common_library)
target_link_libraries(control_plane PRIVATE spdlog::spdlog)
target_link_libraries(control_plane PRIVATE sockpp-static)
target_link_libraries(control_plane PRIVATE redis++::redis++_static)
target_link_libraries(control_plane PRIVATE stduuid)
target_link_libraries(control_plane PRIVATE Crow::Crow)
target_link_libraries(control_plane PRIVATE ${HIREDIS_LIB})
#target_link_libraries(control_plane PRIVATE ${REDIS_PLUS_PLUS_LIB})


###
# Local Worker
###
file(GLOB local_worker_files "system/local-worker/*.cpp")
add_executable(local_worker ${local_worker_files})
set_target_properties(local_worker PROPERTIES RUNTIME_OUTPUT_DIRECTORY bin)
add_dependencies(local_worker cxxopts::cxxopts)
add_dependencies(local_worker sockpp-static)
add_dependencies(local_worker spdlog::spdlog)
add_dependencies(local_worker runtime_library)
add_dependencies(local_worker common_library)
target_include_directories(local_worker SYSTEM PRIVATE $<TARGET_PROPERTY:cxxopts::cxxopts,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(local_worker SYSTEM PRIVATE $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(local_worker SYSTEM PRIVATE $<TARGET_PROPERTY:sockpp-static,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(local_worker SYSTEM PRIVATE $<TARGET_PROPERTY:runtime_library,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(local_worker PRIVATE common_library)
target_link_libraries(local_worker PRIVATE spdlog::spdlog)
target_link_libraries(local_worker PRIVATE sockpp-static)
target_link_libraries(local_worker PRIVATE runtime_library)

