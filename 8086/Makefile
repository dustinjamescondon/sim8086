CC=gcc

main: main.c
	$(CC) -o main main.c

main_dgb: main.c
	$(CC) -o main main.c -g

test: main.c
	$(CC) -o test main.c -g -DTEST
	./test
