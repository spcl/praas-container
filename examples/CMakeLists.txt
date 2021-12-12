
set(examples example)
foreach(example ${examples})
  add_library(${example} SHARED ${example}.cpp)
  set_target_properties(${example} PROPERTIES POSITION_INDEPENDENT_CODE On)
  target_include_directories(${example} SYSTEM PRIVATE $<TARGET_PROPERTY:runtime_library,INTERFACE_INCLUDE_DIRECTORIES>)
endforeach()

set(examples memory)
foreach(example ${examples})
  add_executable(${example} ${example}.cpp)
  set_target_properties(${example} PROPERTIES POSITION_INDEPENDENT_CODE On)
  target_include_directories(${example} SYSTEM PRIVATE $<TARGET_PROPERTY:runtime_library,INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(${example} runtime_library)
endforeach()

configure_file(config.json config.json COPYONLY)
