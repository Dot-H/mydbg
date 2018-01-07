CC ?= gcc
CFLAGS += -Wall -Wextra -Werror -std=c99 -pedantic
LDFLAGS =
LDLIBS =

SRC = classic.c
OBJ = ${SRC:.c=.o}

all: classic

classic: ${OBJ}

check: CFLAGS += -g3 -DTEST
check: classic

.PHONY: clean check
clean:
	${RM} ${OBJ}
	${RM} classic
