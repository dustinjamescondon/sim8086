#include <assert.h>
#include "decode.c"

int main(int argc, char** arcv) {
  {
    printf("First line\n");
    u16 bin = 0b1000100111011001;
    OpCode result = decode(bin);
    Register expRegLeft = CX;
    Register expRegRight = BX;  
    assert(expRegLeft == result.Register2 && "Left register incorrect\n");
    assert(expRegRight == result.Register1 && "Right register incorrect\n");
    print_opcode(&result);
    printf("\n");
  }

  {
    printf("Second line\n");
    u16 bin = 0b1000100011100101;
    OpCode result = decode(bin);
    Register expRegLeft = CH;
    Register expRegRight = AH;  
    assert(expRegLeft == result.Register2 && "Left register incorrect\n");
    assert(expRegRight == result.Register1 && "Right register incorrect\n");
    print_opcode(&result);
  }
  return 0;
}

