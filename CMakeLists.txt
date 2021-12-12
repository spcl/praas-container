
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

add_compile_options(-Wall -Wextra -pedantic)

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
add_dependencies(runtime_library aws-cpp-sdk-s3)
target_include_directories(runtime_library PUBLIC "runtime/include")
target_include_directories(runtime_library SYSTEM PUBLIC $<TARGET_PROPERTY:cxxopts::cxxopts,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PUBLIC $<TARGET_PROPERTY:spdlog::spdlog,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PUBLIC $<TARGET_PROPERTY:sockpp-static,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PUBLIC $<TARGET_PROPERTY:tcpunch,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PUBLIC ${cjson_SOURCE_DIR})
target_include_directories(runtime_library SYSTEM PUBLIC ${threadpool_SOURCE_DIR})
target_include_directories(runtime_library SYSTEM PUBLIC $<TARGET_PROPERTY:aws-cpp-sdk-core,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(runtime_library SYSTEM PUBLIC $<TARGET_PROPERTY:aws-cpp-sdk-s3,INTERFACE_INCLUDE_DIRECTORIES>)
# Header only Boost.interprocess
target_include_directories(runtime_library SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(runtime_library PUBLIC spdlog::spdlog)
target_link_libraries(runtime_library PUBLIC sockpp-static)
target_link_libraries(runtime_library PUBLIC tcpunch)
target_link_libraries(runtime_library PUBLIC cjson-static)
target_link_libraries(runtime_library PUBLIC aws-cpp-sdk-s3 aws-cpp-sdk-core)
target_link_libraries(runtime_library PUBLIC dl)
target_link_libraries(runtime_library PUBLIC rt)

###
# Session exec
###
file(GLOB runtime_session_files "runtime/bin/*.cpp")
add_executable(runtime_session ${runtime_session_files})
set_target_properties(runtime_session PROPERTIES RUNTIME_OUTPUT_DIRECTORY bin)
add_dependencies(runtime_session runtime_library)
target_include_directories(runtime_session SYSTEM PRIVATE $<TARGET_PROPERTY:runtime_library,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(runtime_session PRIVATE runtime_library)

###
# Process exec
###
file(GLOB process_exec_files "runtime/process/*.cpp")
add_executable(process_exec ${process_exec_files})
set_target_properties(process_exec PROPERTIES RUNTIME_OUTPUT_DIRECTORY bin)
target_include_directories(process_exec PUBLIC "runtime/include")
target_include_directories(process_exec PUBLIC "runtime/process")
set(targets runtime_library)
foreach(target ${targets})
  add_dependencies(process_exec ${target})
  target_include_directories(process_exec SYSTEM PRIVATE $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(process_exec PRIVATE ${target})
endforeach()
target_include_directories(process_exec SYSTEM PUBLIC ${threadpool_SOURCE_DIR})

###
# Local memory controller
###
file(GLOB local_memory_files "runtime/local-memory/*.cpp")
add_executable(local_memory ${local_memory_files})
set_target_properties(local_memory PROPERTIES RUNTIME_OUTPUT_DIRECTORY bin)
target_include_directories(local_memory PUBLIC "runtime/local-memory")
set(targets cxxopts::cxxopts TBB::tbb sockpp-static tcpunch spdlog::spdlog cxxopts::cxxopts)
foreach(target ${targets})
  add_dependencies(local_memory ${target})
  target_include_directories(local_memory SYSTEM PRIVATE $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(local_memory PRIVATE ${target})
endforeach()
target_include_directories(local_memory SYSTEM PUBLIC ${threadpool_SOURCE_DIR})

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

###
# PraaS serving
###
file(GLOB praas_serving_files "system/praas-serving/*.cpp")
add_executable(praas_serving ${praas_serving_files})
target_include_directories(praas_serving PUBLIC "runtime/praas-serving")
set_target_properties(praas_serving PROPERTIES RUNTIME_OUTPUT_DIRECTORY bin)
set(targets cxxopts::cxxopts sockpp-static runtime_library common_library spdlog::spdlog)
foreach(target ${targets})
  add_dependencies(praas_serving ${target})
  target_include_directories(praas_serving SYSTEM PRIVATE $<TARGET_PROPERTY:${target},INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(praas_serving PRIVATE ${target})
endforeach()

add_subdirectory(examples)
