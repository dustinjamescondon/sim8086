#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "bit_ops.c"

typedef enum {
  MOV_REG_REG, // register to register
  MOV_IM_REG // immediate to register
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
} Instruction;

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

bool matches(u8 code, u8 pattern, u8 mask) {
  u8 masked_code = code & mask;
  return masked_code == pattern;
}

Operation decode_operation(u8 code) {
  static const u8 reg_to_reg = 0b10001000;
  static const u8 reg_to_reg_mask = 0b11111100;
  
  static const u8 im_to_reg = 0b10110000;
  static const u8 im_to_reg_mask = 0b11110000;

  if(matches(code, reg_to_reg, reg_to_reg_mask)) {
    return MOV_REG_REG;
  } else if(matches(code, im_to_reg, im_to_reg_mask)) {
    return MOV_IM_REG;
  }
  
  return 0;
}

Instruction decode(u16 code) {
  Operation operation = decode_operation(upper(code));

  switch (operation) {
  case(MOV_REG_REG):
    {
      static const u16 REG1_MASK = 0x0038; // 00000000,00111000
      static const u16 REG2_MASK = 0x0007; // 00000000,00000111
      static const u16 REG1_SHIFT = 3;
      static const u16 REG2_SHIFT = 0;
      static const u16 W_MASK = 0x0100;   // 00000001,00000000
    
      u8 reg1 = (code & REG1_MASK) >> REG1_SHIFT;
      u8 reg2 = (code & REG2_MASK) >> REG2_SHIFT;
      u16 w = code & W_MASK;

      Register first = w ? decode_w1(reg1) : decode_w0(reg1);
      Register second = w ? decode_w1(reg2) : decode_w0(reg2);
    
      Instruction result;
      result.Operation = MOV_REG_REG;
      result.Register1 = first;
      result.Register2 = second;
      return result;
    }
  case MOV_IM_REG:
    {
      static const u16 REG_MASK = 0x0300;
      static const u16 W_MASK = 0x0100;   // 00001000,00000000
      u16 w = code & W_MASK;
    }
  default:
    assert(false);
    break;
  }    
}

void print_instruction(const Instruction* opcode) {  
  printf("mov %s, %s", reg_names[opcode->Register2], reg_names[opcode->Register1]);
}
