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

  // First read into the buffer
  static const u16 max_buffer_size = 1000;
  u8 buffer[max_buffer_size];
  u16 buffer_size = 0;
  {
    u16 buffer_pos = 0;
    while(fread(buffer + buffer_pos, sizeof(u8), 1, fptr)) {
      buffer_pos = buffer_pos + 1;
    }
    buffer_size = buffer_pos + 1;
  }

  assert(max_buffer_size >= buffer_size && "Buffer too small for the file");

  // Next decode the buffer
  u16 current_buffer_pos = 0;
  while(current_buffer_pos < buffer_size - 1) {
    u16 move = 0;
    char assembly[500];
    decode(buffer + current_buffer_pos, &move, assembly);
    printf("%s\n", assembly);
    current_buffer_pos = current_buffer_pos + move;
  }
  
  return 0;
}
