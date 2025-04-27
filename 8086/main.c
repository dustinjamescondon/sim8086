#include "decode.c"

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

  printf("bits 16\n\n");
  
  u16 buffer;
  while(fread(&buffer, sizeof(u16), 1, fptr)) {
    u16 _buffer = join(lower(buffer), upper(buffer));
    OpCode code = decode(_buffer);
    print_opcode(&code);
    printf("\n");
  }
  return 0;
}
