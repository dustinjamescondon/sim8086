CC=gcc

main: main.c decode.c
	$(CC) -o main main.c

run: main
	./main listing_37

main_dgb: main.c decode.c
	$(CC) -o main main.c -g

test: test.c decode.c
	$(CC) -o test test.c -g -DTEST
	./test

read_test: read_test.c decode.c
	$(CC) -o read_test read_test.c -g -DTEST
	./read_test listing_37

test_dbg: test.c decode.c 
	$(CC) -o test test.c -g -DTEST
