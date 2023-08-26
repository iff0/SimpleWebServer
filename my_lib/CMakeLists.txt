
file(GLOB_RECURSE SRC "./src/*.cpp")
foreach(source ${SRC})
    message(STATUS "Found source file: ${source}")
endforeach()
message("shit")
add_subdirectory(fmt EXCLUDE_FROM_ALL)

add_library(my_lib STATIC ${SRC} )
target_link_libraries(my_lib PUBLIC fmt::fmt)
target_include_directories(my_lib PUBLIC ./include)
#set_target_properties(my_lib PROPERTIES LINKER_LANGUAGE CXX)