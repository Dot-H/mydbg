cmake_minimum_required(VERSION 2.8)

project(MY_DBG C)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -std=c99 -pedantic -D_DEFAULT_SOURCE \
        -DDEBUG")#-fsanitize=address")
include_directories("src/" "src/opening" "src/inputs" "src/commands" "\
        src/tracing")

set(SRC
        src/commands/commands.c
        src/commands/help.c
        src/commands/quit.c
        src/inputs/inputs.c
        src/my_dbg.c
        src/opening/mapping.c
        src/tracing/trace.c
  )

add_executable(my_dbg ${SRC})
include_directories(src src/opening)
target_link_libraries(my_dbg readline)
