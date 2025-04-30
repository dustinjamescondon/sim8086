CC=gcc

sim8086: main.c decode.c bit_ops.c
	$(CC) -o sim8086 main.c

sim8086_dbg: main.c decode.c
	$(CC) -o sim8086 main.c -g

unit_test: unit_test.c decode.c bit_ops.c
	$(CC) -o unit_test unit_test.c -g -rdynamic
	./unit_test
