#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define i16 int16_t
#define u16 uint16_t
#define u8 char
#define W_MASK 0x0100   // 00000001,00000000
#define W_SHIFT 8
#define REG1_MASK 0x0038 // 00000000,00111000
#define REG2_MASK 0x0007 // 00000000,00000111
#define REG1_SHIFT 3
#define REG2_SHIFT 0
#define OPCODE_MASK 0xFC00
#define OPCODE_SHIFT 10

typedef enum {
  MOV
} Operation;

typedef enum {
  AL = 0,
  CL,
  DL,
  BL,
  AH,
  CH,
  DH,
  BH,
  AX,
  CX,
  DX,
  BX,
  SP,
  BP,
  SI,
  DI
} Register;

const char* reg_names[] = {
  "al",
  "cl",
  "dl",
  "bl",
  "ah",
  "ch",
  "dh",
  "bh",
  "ax",
  "cx",
  "dx",
  "bx",
  "sp",
  "bp",
  "si",
  "di"
};

typedef struct{
  Operation Operation;
  Register Register1;
  Register Register2;
} OpCode;

Register decode_w0(u8 reg) {
  switch (reg)
    {
    case 0x00:
      return AL;
    case 0x01:
      return CL;
    case 0x02:
      return DL;
    case 0x03:
      return BL;
    case 0x04:
      return AH;
    case 0x05:
      return CH;
    case 0x06:
      return DH;
    case 0x07:
      return BH;
    default:
      return  -1;
    }
}

Register decode_w1(u8 reg) {
  switch (reg)
    {
    case 0x00:
      return AX;
    case 0x01:
      return CX;
    case 0x02:
      return DX;
    case 0x03:
      return BX;
    case 0x04:
      return SP;
    case 0x05:
      return BP;
    case 0x06:
      return SI;
    case 0x07:
      return DI;
    default:
      exit(1);
      return -1;
    }
}

OpCode decode(u16 code) {
  u8 reg1 = (code & REG1_MASK) >> REG1_SHIFT;
  u8 reg2 = (code & REG2_MASK) >> REG2_SHIFT;
  u8 w = code & W_MASK >> W_SHIFT;

  Register first = w ? decode_w1(reg1) : decode_w0(reg1);
  Register second = w ? decode_w1(reg2) : decode_w0(reg2);
  OpCode result;
  result.Operation = MOV;
  result.Register1 = first;
  result.Register2 = second;
  return result;
}

void print_opcode(const OpCode* opcode) {  
  printf("mov %s, %s", reg_names[opcode->Register1], reg_names[opcode->Register2]);
}

#ifndef TEST
int main(int argc, char *argv[]) {
#if 0
  if(argc <= 1) {
    printf("file arg not provided\n");
    return -1;
  }
  const char* filename = argv[1];
#else 
  const char* filename = "listing_37";
#endif

  FILE *fptr = NULL;
  fptr = fopen(filename, "rb");

  if(fptr == NULL) {
    printf("Couldn't open the file %s\n", filename);
    return -1;
  }

  u16 buffer;
  while(fread(&buffer, sizeof(u16), 1, fptr)) {
    OpCode code = decode(buffer);
    print_opcode(&code);
    printf("\n");
  }
  return 0;
}

#else
#include <assert.h>

int main(int argc, char** arcv) {

  u16 bin = 0b1000100111011001;
  OpCode result = decode(bin);
  Register expRegLeft = CX;
  Register expRegRight = BX;  
  assert(expRegLeft == result.Register2 && "Left register incorrect");
  assert(expRegRight == result.Register1 && "Right register incorrect");
  return 0;
}
#endif
