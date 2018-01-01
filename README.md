mydbg 0.3
=========

Description
-----------

    This project is gdb-like debugger for 64bits elf executables.

Usage
-----

    ./my_dbg [FILE]

    In order to use the debugger, simply run the my_dbg binary. If
    an argument is provided, this must be a 64bits elf. The argument
    will become the working binary. If desired, it can be overloaded
    via the file command.

    __Example:__

        ./my_dbg super_program

Commands
--------

    help    [CMD]   Print documentation for every command if no argument
                    is specified. Otherwise, print the documentation of
                    command in argument.

    quit            Quit mydbg.

    run     [FILE]  Run the currently loaded binary if no argument given.
                    Otherwise, the argument is loaded and run.

    info_process    Print all the running process.

    info_regs       Print the registers of the current process

    info_header     Print the elf header of the current loaded file

    file    FILE    Load binary given in argument.

    break   [ADDR]  Put a breakpoint on the address in argument if any and
                    on the current address otherwise.

    tbreak  [ADDR]  Put a temporary breakpoint on the address in argument if
                    any and on the current address otherwise. A temporary is
                    hit only one time before being trash.

    breakf  [FUNC]  Put a breakpoint on the address of the function given in
                    argument.

    break_list      List all the breakpoints

    break_del
            [ID]    Delete the breakpoint corresponding to the id given in
                    argument.

    continue
            [PID]   Continue the execution of the pid given in argument.

    singlestep
            [PID]   Execute a unique instruction

    examine $format size start_addr [PID]
                    Print size bytes from start_addr with the format given in
                    argument.

            $i      Disassemble

            $d      integer format

            $x      hexadecimal format (32bit)

            $s      null terminated string

    backtrace
            [PID]   Print the call stack from the current addr

    list    [N]     List N lines starting from the current one. N is by default
                    set to 10 and must be an integer.

    step_line       Continue the execution of the tracee until the next line
                    or subcall.

    next_line       Continue the execution of the tracee until the next line
                    without stopping on the subcalls.

Return values
-------------

    2   argument error
    1   error while processing the executable
    0   everything went fine

Notes
-----

    Lists are from "https://github.com/wayland-project/wayland".
    Copyrights in wayland-util.h and wayland-util.c.

    A history file named .mydbg_history is written in $HOME. The name can
    be modified in my_dbg.h

AUTHORS
-------

    Alexandre 'DotH' Bernard
