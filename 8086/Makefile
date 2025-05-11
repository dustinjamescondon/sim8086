CC=g++

sim8086: main.cpp decode.cpp bit_ops.cpp
	$(CC) -o sim8086 main.cpp

sim8086_dbg: main.cpp decode.cpp
	$(CC) -o sim8086 main.cpp -g

unit_test: unit_test.cpp decode.cpp bit_ops.cpp
	$(CC) -o unit_test unit_test.cpp -g -rdynamic
	./unit_test
