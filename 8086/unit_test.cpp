#include <assert.h>
#include "decode.cpp"

void print_hex(u16 value) {
    printf("%hx", value);
}

int main(int argc, char** arcv) {

  { printf("Testing lower\n");
    u16 bin = 0x89d9;
    u8 exp_upper = 0x89;
    u8 exp_lower = 0xd9;

    printf("Expected: ");
    print_hex((u16)exp_lower);
    printf("\n");
    printf("Actual: ");
    print_hex((u16)lower(bin));
    printf("\n");
    
    assert(exp_lower == lower(bin) && "lower incorrect\n");

    printf("\n");
  }

  { printf("Testing upper\n");
    u16 bin = 0x89d9;
    u8 exp_upper = 0x89;
    
    printf("Expected: ");
    print_hex((u16)exp_upper);
    printf("\n") ;   
    printf("Actual: ");
    print_hex((u16)upper(bin));
    printf("\n");
    
    assert(exp_upper == upper(bin) && "upper incorrect\n");

    printf("\n");
  }
  
  { printf("Testing joining\n");
    u16 exp = 0x89d9;
    u16 joined = join(lower(exp), upper(exp));
    
    printf("Expected: ");
    print_hex(exp);
    printf("\n") ;   
    printf("Actual: ");
    print_hex(joined);
    printf("\n");
    
    assert(exp == joined && "join incorrect\n");

    printf("\n");
  }
  
  { printf("First line\n");
    u16 bin = 0b1000100111011001;
    Instruction result = decode(bin);
    Register expRegLeft = CX;
    Register expRegRight = BX;  
    assert(expRegLeft == result.Register2 && "Left register incorrect\n");
    assert(expRegRight == result.Register1 && "Right register incorrect\n");
    print_opcode(&result);
    printf("\n");
  }

  { printf("Second line\n");
    u16 bin = 0b1000100011100101;
    Instruction result = decode(bin);
    Register expRegLeft = CH;
    Register expRegRight = AH;  
    assert(expRegLeft == result.Register2 && "Left register incorrect\n");
    assert(expRegRight == result.Register1 && "Right register incorrect\n");
    print_opcode(&result);
    printf("\n");
  }

 
  return 0;
}

