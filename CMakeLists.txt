cmake_minimum_required(VERSION 3.22)
project(cpp_simple_web_server)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(. src)
#aux_source_directory(test test)

add_executable(${PROJECT_NAME} ${src})
#add_executable(my_test ${test})

message(${src})

add_subdirectory(my_lib)

target_link_libraries(${PROJECT_NAME}  PRIVATE my_lib )
#target_link_libraries(my_test  PRIVATE my_lib )
#target_include_directories(${PROJECT_NAME} PRIVATE "my_lib/include")