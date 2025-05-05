#!/bin/bash
make sim8086
mkdir -p test

run_test() {
    echo -n "Testing $1: "
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
run_test listing_39.1
run_test listing_39.2
run_test listing_39.3
run_test listing_39.4
run_test listing_39
