#!/bin/bash
make sim8086
mkdir -p test

echo -n "Testing listing 38: "

./sim8086 listing_38 > test/listing_38.asm
nasm test/listing_38.asm
xxd test/listing_38 > test/38_original.hex
xxd listing_38 > test/38_after.hex
diff test/38_original.hex test/38_after.hex > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "FAILED"
else
    echo "PASSED"
fi
