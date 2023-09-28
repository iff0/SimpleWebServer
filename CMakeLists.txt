cmake_minimum_required(VERSION 3.22)
#cmake_minimum_required(VERSION 3.10)
project(cpp_simple_web_server)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE release CACHE STRING "Build Type" FORCE)
endif()

aux_source_directory(. src)
#aux_source_directory(test test)

add_executable(${PROJECT_NAME} ${src})
#add_executable(my_test ${test})

message(${src})

add_subdirectory(my_lib)

target_link_libraries(${PROJECT_NAME}  PRIVATE my_lib )
#target_link_libraries(${PROJECT_NAME}  PRIVATE my_lib -lm -lpthread)
#target_link_libraries(my_test  PRIVATE my_lib )
#target_include_directories(${PROJECT_NAME} PRIVATE "my_lib/include")