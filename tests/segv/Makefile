CC ?= gcc
CFLAGS += -Wall -Wextra -Werror -std=c99 -pedantic
LDFLAGS =
LDLIBS =

SRC = segv.c
OBJ = ${SRC:.c=.o}

all: segv

segv: ${OBJ}

check: CFLAGS += -g3 -DTEST
check: segv

.PHONY: clean check
clean:
	${RM} ${OBJ}
	${RM} segv
