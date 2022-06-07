# Make lab3.c
CC = gcc
INC1 = -I .


all:test.c
	${CC} -o test test.c ${INC1} -lncurses
	${CC} -o test2 test2.c ${INC1} sockop.c -lncurses -lpthread
	${CC} -o client client.c ${INC1} sockop.c -lncurses

clean:
	rm -f test 