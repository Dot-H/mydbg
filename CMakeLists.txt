cmake_minimum_required(VERSION 2.8)

project(MY_DBG C)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -std=c99 -pedantic -D_DEFAULT_SOURCE \
        -lunwind-ptrace -lunwind-x86_64 -lunwind")#-fsanitize=address")
include_directories("src/" "src/elf" "src/inputs" "src/commands" "\
        src/tracing" "src/utils" "src/dbgprocess" "src/breakpoint" "\
        src/print" "src/maps")

set(SRC
        src/breakpoint/bp_hit.c
        src/breakpoint/breakpoint.c
        src/commands/backtrace.c
        src/commands/break.c
        src/commands/break_del.c
        src/commands/break_list.c
        src/commands/breakf.c
        src/commands/breaks.c
        src/commands/commands.c
        src/commands/continue.c
        src/commands/examine.c
        src/commands/file.c
        src/commands/finish.c
        src/commands/help.c
        src/commands/info_process.c
        src/commands/info_regs.c
        src/commands/info_header.c
        src/commands/list.c
        src/commands/quit.c
        src/commands/run.c
        src/commands/next_instr.c
        src/commands/singlestep.c
        src/commands/step_line.c
        src/commands/next_line.c
        src/dbgprocess/dproc.c
        src/inputs/inputs.c
        src/maps/maps.c
        src/my_dbg.c
        src/elf/debug.c
        src/elf/mapping.c
        src/elf/symbols.c
        src/print/print_func.c
        src/tracing/trace.c
        src/utils/args_helper.c
        src/utils/hash_table.c
        src/utils/wayland-util.c
  )

add_executable(my_dbg ${SRC})
target_link_libraries(my_dbg readline unwind capstone)
