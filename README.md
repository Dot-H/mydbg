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

    run     [...]  Run the currently loaded binary if no argument given.
                    Otherwise, the currennt binary is run with the given
                    arguments.

    file    FILE [...]
                    Load binary given as first argument and store
                    the rest of the arguments in order to run the binary
                    with it later.

    attach  PID     Attach to the process pointed by pid

    info_process    Print all the running process.

    info_regs       Print the registers of the current process

    info_header     Print the elf header of the current loaded file

    break   [ADDR]  Put a breakpoint on the address in argument if any and
                    on the current address otherwise.

    tbreak  [ADDR]  Put a temporary breakpoint on the address in argument if
                    any and on the current address otherwise. A temporary is
                    hit only one time before being trash.

    breakf  FUNC  Put a breakpoint on the address of the function given in
                    argument.

    breaks  SYSNO   Put a breakpoint at the entry of the syscall number given
                    in argument. The debugger will also stop the process when
                    returning from the syscall

    breakl  LINE [FILE]
                    Put a breakpoint at the line given in argument in the
                    current file or the one given in argument. If no file is
                    given in argument my_dbg tries to get the current one
                    from the current address.

    watchpoint
            [ADDR] [COND] [LEN] [PID]
                    Put a watchpoint at the address given in argumnent or at
                    the current address if no argument. COND represents the
                    exception condition of the watchpoint. LEN represents the
                    number of watched bytes. PID represents the process.
                    Possible values of COND are 'w' (write) 'rw' (read | right)
                    and the default value is 'w'.
                    Possible values of LEN are '1' (1 byte) '2' (2 byte) '4'
                    (4 bytes) and '8' (8 bytes).

    break_list      List all the breakpoints

    break_del
            [[dr]ID]
                    Delete the breakpoint corresponding to the id given in
                    argument. If the breakpoint is an hardware breakpoint,
                    the id must be preceded by 'dr'

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

    list    [N] [ADDR]
                    List N lines starting from the current one or the addr
                    given in argument. N is by default set to 10 and must
                    be an unsigned integer. Note that to give an address,
                    N must be given too.

    step_line       Continue the execution of the tracee until the next line
                    or subcall.

    next_line       Continue the execution of the tracee until the next line
                    without stopping on the subcalls.

    disas   [N] [ADDR] [PID]
                    Disas N instructions starting from the current addr or
                    the one given in argument. N is by default set to 10 and
                    must be an unsigned integer. Note that to give an address,
                    N must be given too.

Return values
-------------

    2   argument error
    1   error while processing the executable
    0   everything went fine

Notes
-----

    Lists are from "https://github.com/wayland-project/wayland".
    Copyrights in wayland-util.h and wayland-util.c.

    A history file named .mydbg_history is written at $HOME. The name can
    be modified in my_dbg.h

AUTHORS
-------

    Alexandre 'DotH' Bernard (bernar_1)
