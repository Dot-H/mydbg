mydbg 0.1
=========

Description
-----------

    This project is gdb-like debugger for 64bits elf executables.

Usage
-----

    In order to use the debugger, simply run the binary. If an
    argument is provided, this must be a 64bits elf.

    __Example:__

        ./my_dbg elf

Commands
--------

    help    Print documentation for every command
    quit    Quit mydbg
    run     Run the currently loaded binary

Return values
-------------

    2   argument error
    1   error while processing the executable
    0   everything went fine

Notes
-----

    Array and lists are from "https://github.com/wayland-project/wayland".
    Copyrights in wayland-util.h and wayland-util.c.

    A history file named .mydbg_history is written in $HOME. The name can
    be modified in my_dbg.h

AUTHORS
-------

    Alexandre 'DotH' Bernard
