#include <assert.h>
#include "decode.c"

int main(int argc, char** arcv) {
  u16 bin = 0b1000100111011001;
  OpCode result = decode(bin);
  Register expRegLeft = CX;
  Register expRegRight = BX;  
  assert(expRegLeft == result.Register2 && "Left register incorrect");
  assert(expRegRight == result.Register1 && "Right register incorrect");
  print_opcode(&result);
  return 0;
}

