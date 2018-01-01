CC ?= gcc
CFLAGS += -Wall -Wextra -Werror -std=c99 -pedantic -g

SRC = main.c a.c b.c c.c
OBJ = ${SRC:.c=.o}

all: main

main: ${OBJ}

.PHONY: clean check
clean:
	${RM} ${OBJ}
	${RM} main
