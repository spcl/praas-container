
set(examples example)
foreach(example ${examples})
  add_library(${example} SHARED ${example}.cpp)
  set_target_properties(${example} PROPERTIES POSITION_INDEPENDENT_CODE On)
  target_include_directories(${example} SYSTEM PRIVATE $<TARGET_PROPERTY:runtime_library,INTERFACE_INCLUDE_DIRECTORIES>)
endforeach()

configure_file(config.json config.json COPYONLY)
