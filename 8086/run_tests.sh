#!/bin/bash
make sim8086
mkdir -p test

run_test() {
    echo -n "Testing $1: "
    
    # if the binary doesn't exist, generate it with nasm
    if [ ! -f $1.asm ]; then
	nasm $1.asm
    fi
    
    ./sim8086 $1 > test/$1.asm
    nasm test/$1.asm
    xxd test/$1 > test/$1_result.hex
    xxd $1 > test/$1_original.hex
    the_diff="$(diff test/$1_original.hex test/$1_result.hex)"
    if [ $? -ne 0 ]; then
	echo "FAILED"
	echo $the_diff
    else
	echo "PASSED"
    fi    
}

run_test listing_38
run_test listing_39
run_test listing_41.1
