
set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

###
# cxxopts
###
find_package(cxxopts QUIET)
if(NOT cxxopts_FOUND)
  message(STATUS "Downloading and building cxxopts dependency")
  FetchContent_Declare(cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    CMAKE_ARGS -DCXXOPTS_BUILD_EXAMPLES=Off -DCXXOPTS_BUILD_TESTS=Off
  )
  FetchContent_Populate(cxxopts)
  FetchContent_MakeAvailable(cxxopts)
  add_subdirectory(${cxxopts_SOURCE_DIR} ${cxxopts_BINARY_DIR})
endif()

###
# sockpp
###
find_package(sockpp QUIET)
if(NOT sockpp_FOUND)
  message(STATUS "Downloading and building sockpp dependency")
  FetchContent_Declare(sockpp
    GIT_REPOSITORY https://github.com/spcl/sockpp
  )
  FetchContent_Populate(sockpp)
  FetchContent_MakeAvailable(sockpp)
  set(SOCKPP_BUILD_SHARED OFF CACHE INTERNAL "Build SHARED libraries")
  set(SOCKPP_BUILD_STATIC ON CACHE INTERNAL "Build SHARED libraries")
  add_subdirectory(${sockpp_SOURCE_DIR} ${sockpp_BINARY_DIR})
endif()

###
# spdlog
###
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
  message(STATUS "Downloading and building spdlog dependency")
  FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    # default branch is v1.x - for some reason, cmake switches to master
    GIT_TAG v1.8.0
  )
  FetchContent_Populate(spdlog)
  FetchContent_MakeAvailable(spdlog)
  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
else()
  add_custom_target(spdlog)
endif()

###
# threadpool
###
FetchContent_Declare(threadpool
  GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
  GIT_TAG master
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
)
FetchContent_GetProperties(threadpool)
if(NOT threadpool_POPULATED)
  FetchContent_Populate(threadpool)
endif()

###
# redis
###
find_path(HIREDIS_HEADER hiredis)
find_library(HIREDIS_LIB hiredis)
find_package(redis QUIET)
if(NOT redis_FOUND)
  message(STATUS "Downloading and building redis-plus-plus dependency")
  FetchContent_Declare(redis
    GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
    GIT_TAG master
  )
  FetchContent_Populate(redis)
  FetchContent_MakeAvailable(redis)
  add_subdirectory(${redis_SOURCE_DIR} ${redis_BINARY_DIR})
else()
  add_custom_target(redis)
endif()
# required by the library specifically - otherwise the header paths are not correct
#find_path(REDIS_PLUS_PLUS_HEADER sw)
#find_library(REDIS_PLUS_PLUS_LIB redis++)

###
# stduuid
###
find_package(stduuid QUIET)
if(NOT stduuid_FOUND)
  message(STATUS "Downloading and building stduuid dependency")
  FetchContent_Declare(stduuid
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
  )
  FetchContent_Populate(stduuid)
  FetchContent_MakeAvailable(stduuid)
  add_subdirectory(${stduuid_SOURCE_DIR} ${stduuid_BINARY_DIR})
endif()

###
# tcpunch library
###
find_package(tcpunch QUIET)
if(NOT tcpunch_FOUND)
  message(STATUS "Downloading and building tcpunch dependency")
  FetchContent_Declare(tcpunch
    GIT_REPOSITORY  https://$ENV{GITHUB_ACCESS_TOKEN}@github.com/mcopik/TCPunch.git
    GIT_TAG         main
    SOURCE_SUBDIR   client
  )
  FetchContent_Populate(tcpunch)
  add_subdirectory(${tcpunch_SOURCE_DIR}/client ${tcpunch_BINARY_DIR})
endif()

###
# crow
###
find_package(Crow QUIET)
if(NOT Crow_FOUND)
  message(STATUS "Downloading and building crow dependency")
  FetchContent_Declare(Crow
    GIT_REPOSITORY  https://github.com/CrowCpp/Crow.git
    #GIT_TAG         v0.3+3
  )
  FetchContent_Populate(Crow)
  FetchContent_MakeAvailable(Crow)
  set(CROW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
  set(CROW_BUILD_TESTS  OFF CACHE INTERNAL "")
  set(CROW_ENABLE_SSL ON CACHE INTERNAL "")
  add_subdirectory(${crow_SOURCE_DIR} ${crow_BINARY_DIR})
endif()

###
# cJSON
###
find_package(cJSON QUIET)
if(NOT cJSON_FOUND)
  message(STATUS "Downloading and building cJSON dependency")
  FetchContent_Declare(cJSON
    GIT_REPOSITORY  https://github.com/DaveGamble/cJSON
  )
  FetchContent_Populate(cJSON)
  set(ENABLE_CJSON_TEST OFF CACHE INTERNAL "")
  set(CJSON_BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
  set(BUILD_SHARED_AND_STATIC_LIBS ON CACHE INTERNAL "")
  FetchContent_MakeAvailable(cJSON)
  add_subdirectory(${cjson_SOURCE_DIR} ${cjson_BINARY_DIR})
endif()

###
# AWS SDK - just the S3 part.
###
find_package(AWSSDK COMPONENTS s3 QUIET)
if(NOT AWSSDK_FOUND)
  message(STATUS "Downloading and building AWS SDK dependency")
  FetchContent_Declare(awsSDK
    GIT_REPOSITORY  https://github.com/aws/aws-sdk-cpp
  )
  FetchContent_Populate(awsSDK)
  set(BUILD_ONLY "s3" CACHE INTERNAL "")
  set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
  set(ENABLE_TESTING OFF CACHE INTERNAL "")
  FetchContent_MakeAvailable(awsSDK)
  add_subdirectory(${awssdk_SOURCE_DIR} ${awssdk_BINARY_DIR})
else()
  message("Found AWS SDK at: ${awssdk_DIR} ${AWSSDK_DIR}")
endif()

###
# Boost interprocess - we use it for stream buffer.
###
find_package(Boost REQUIRED)

###
# TBB - static libraries are discouraged
###
find_package(TBB COMPONENTS s3 QUIET)
if(NOT TBB_FOUND)
  message(STATUS "Downloading and building TBB dependency")
  FetchContent_Declare(TBB
    GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
  )
  FetchContent_Populate(TBB)
  set(TBB_TEST OFF CACHE INTERNAL "")
  set(BUILD_SHARED_LIBS ON CACHE INTERNAL "")
  FetchContent_MakeAvailable(TBB)
  add_subdirectory(${tbb_SOURCE_DIR} ${tbb_BINARY_DIR})
endif()

###
# google test
###
if(${RFAAS_WITH_TESTING})
  include(FetchContent)
  message(STATUS "Downloading and building gtest")
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

