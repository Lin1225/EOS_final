# Make lab3.c
CC = arm-unknown-linux-gnu-gcc
LIB = -L /opt/arm-unknown-linux-gnu/arm-unknown-linux-gnu/lib
INC1 = -I /opt/arm-unknown-linux-gnu/arm-unknown-linux-gnu/include
INC2 = -I /home/julie/microtime/linux/include


all:snake.c
	${CC} -o snake snake.c ${LIB} ${INC1} ${INC2}  sockop.c -lpthread
	${CC} -o snake_client snake_client.c ${LIB} ${INC1} ${INC2}  sockop.c -lpthread

clean:
	rm -f snake snake_client