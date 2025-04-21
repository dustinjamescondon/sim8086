#include "decode.c"
#include "assert.h"

void print_hex(u16 value) {
    printf("%hx", value);
}

int main(int argc, char *argv[]) {
  if(argc <= 1) {
    printf("file arg not provided\n");
    return -1;
  }
  const char* filename = argv[1];

  FILE *fptr = NULL;
  fptr = fopen(filename, "rb");

  if(fptr == NULL) {
    printf("Couldn't open the file %s\n", filename);
    return -1;
  }

  { // little aside test
    u16 value = 0x9669;
    u8 LOWER = 0x69;
    u8 UPPER = 0x96;
    u16 joined = join(UPPER, LOWER);
  
    printf("before join: "); print_hex(value); printf("\n");
    printf("after join: "); print_hex(joined); printf("\n");

    print_hex(lower(value)); printf("\n");
    assert(LOWER == lower(value) && "lower not working");
    print_hex(upper(value)); printf("\n");
    assert(UPPER == upper(value) && "upper not working");
    assert(joined == value && "join not working");
  }
  
  printf("reading from %s...\n", filename);
  u16 read_value = 0;
  // Read in everything first
  while(fread(&read_value, 2, 1, fptr)) {
    // read value will have different endian-ness than what my CPU is, so flip it
    print_hex(read_value); printf("\n");
    u16 opcode = join(lower(read_value), upper(read_value));
    u16 expected = 0b1000100111011001;
    
    print_hex(opcode); printf("\n");
    print_hex(expected); printf("\n");
    assert(expected == opcode && "Opcode wrong");
   
    OpCode code = decode(opcode);
    print_opcode(&code);
    printf("\n");   
  }
  fclose(fptr);  
  return 0;
}
