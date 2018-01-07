CC ?= gcc
CFLAGS += -Wall -Wextra -Werror -std=c99 -pedantic
LDFLAGS =
LDLIBS +=

TST = test.c
OBJ_TST = ${TST:.c=.o}

BT = backtrace.c
OBJ_BT = ${BT:.c=.o}

all: test

test: ${OBJ}

check: CFLAGS += -g3 -DTEST -o test_dbg
check: test

nopie: CFLAGS += -no-pie -o test_nopie
nopie: LDFLAGS += -no-pie
nopie: test

backtrace_dbg: CFLAGS += -g
backtrace_dbg: backtrace

backtrace: ${OBJ_BT}
backtrace:
	${CC} ${CFLAGS} ${OBJ_BT} -o backtrace

.PHONY: clean check backtrace nopie backtrace_dbg
clean:
	${RM} ${OBJ}
	${RM} ${OBJ_TST}
	${RM} ${OBJ_BT}
	${RM} test
	${RM} backtrace
