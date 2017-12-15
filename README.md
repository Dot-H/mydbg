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
    quit    Quits the debugger

Return values
-------------

    2   argument error
    1   error while processing the file
    0   everything went fine

Notes
-----

    A history file is written in $HOME.

AUTHORS
-------

    Alexandre 'DotH' Bernard
