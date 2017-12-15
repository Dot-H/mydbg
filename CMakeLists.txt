cmake_minimum_required(VERSION 2.8)

project(MY_DBG C)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -std=c99 -pedantic -fsanitize=address")
include_directories("src/" "src/opening")

set(SRC
        src/opening/mapping.c
        src/my_dbg.c
  )

add_executable(my_dbg ${SRC})
include_directories(src src/opening)
target_link_libraries(my_dbg readline)
