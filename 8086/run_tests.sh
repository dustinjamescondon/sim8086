#!/bin/bash
make sim8086
mkdir -p test

run_test() {
    echo -n "Testing $1: "
    
    # if the binary doesn't exist, generate it with nasm
    if [ ! -f listings/$1 ]; then
    	nasm listings/$1.asm -o listings/$1
    fi
    
    ./sim8086 listings/$1 > "test/$1.asm"
    if [ $? -ne 0 ]; then
    	echo "FAILED (couldn't decode)"
	exit 1
    fi    
    
    nasm "test/$1.asm" -o "test/$1"
    xxd test/$1 > test/$1_result.hex
    xxd listings/$1 > test/$1_original.hex
    the_diff="$(diff test/$1_original.hex test/$1_result.hex)"
    if [ $? -ne 0 ]; then
    	echo "FAILED"
    else
    	echo "PASSED"
    fi    
}

run_test listing_38
run_test listing_39
run_test listing_41.1
run_test listing_41.3
run_test listing_41.nojump
