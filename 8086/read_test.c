#include "decode.c"
#include "assert.h"

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

  u16 buffer;
  while(fread(&buffer, sizeof(u16), 1, fptr)) {
    printf("%#18x\n", buffer);
    OpCode code = decode(buffer);
    print_opcode(&code);
    printf("\n");
  }

  u16 expected = 0b1000100111011001;

  assert(expected == buffer);
  return 0;
}
