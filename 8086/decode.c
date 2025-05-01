#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "bit_ops.c"

typedef enum {
  MOV_MEM_REG =        0x00, // memory address to register (MOD 00)
  MOV_MEM_REG_DISP8  = 0x01, //                            (MOD 01)
  MOV_MEM_REG_DISP16 = 0x02, //                            (MOD 10)
  MOV_REG_REG =        0x03, // register to register       (MOD 11)

  MOV_IM_REG,         // immediate to register
} Operation;

const char* w0_reg_names[] = {    
  "al",
  "cl",
  "dl",
  "bl",
  "ah",
  "ch",
  "dh",
  "bh"
};

const char* w1_reg_names[] = {    
  "ax",
  "cx",
  "dx",
  "bx",
  "sp",
  "bp",
  "si",
  "di"
};

bool matches(u8 code, u8 pattern, u8 mask) {
  u8 masked_code = code & mask;
  return masked_code == pattern;
}

Operation decode_operation(const u8 *buffer) {
  static const u8 reg_to_reg = 0b10001000;
  static const u8 reg_to_reg_mask = 0b11111100;
  
  static const u8 im_to_reg = 0b10110000;
  static const u8 im_to_reg_mask = 0b11110000;

  static const u8 MOD_MASK = 0b11000000;
  static const u8 MOD_SHIFT = 6;

  if(matches(buffer[0], reg_to_reg, reg_to_reg_mask)) {
    u8 mod = (buffer[1] & MOD_MASK) >> MOD_SHIFT;

    switch(mod) {
    case 0x00:
      return MOV_MEM_REG;
    case 0x01:
      return MOV_MEM_REG_DISP8;
    case 0x02:
      return MOV_MEM_REG_DISP16;
    case 0x04:
    default:
      return MOV_REG_REG;
    }
    
  } else if(matches(buffer[0], im_to_reg, im_to_reg_mask)) {
    return MOV_IM_REG;
  }
  
  return 0;
}

// Fills out the result field with the assembly
void decode(const u8* buffer, u16* move, char result[]) {
  Operation operation = decode_operation(buffer);

  static const u16 REG_MASK = 0x0038; // 00000000,00111000
  static const u16 REG_SHIFT = 3;
  static const u8 D_MASK = 0b00000010;
  static const u8 RM_MASK = 0b00000111;
  static const u8 RM_SHIFT = 0;
  
  u8 rm = (buffer[1] & RM_MASK) >> RM_SHIFT;
  u16 code = join(buffer[1], buffer[0]);
      
  u8 reg1 = (code & REG_MASK) >> REG_SHIFT;
  u8 reg2 = (code & RM_MASK) >> RM_SHIFT;
  
  static const u16 W_MASK = 0x01;   // 00000001
  u16 w = buffer[0] & W_MASK;

  const char* second_reg = w ? w1_reg_names[reg2] : w0_reg_names[reg2];
  
  switch (operation) {

  case(MOV_REG_REG):
    {
      const char* first_reg = w ? w1_reg_names[reg1] : w0_reg_names[reg1];
     
      sprintf(result, "mov %s, %s", second_reg, first_reg);
      
      *move = 2;
      return;
    }
    break;
  case(MOV_MEM_REG):
    {
      // Index these by r/m
      static const char * MOD00[] = {
	"[bx + si]",
	"[bx + di]",
	"[bp + si]",
	"[bp + di]",
	"si",
	"di",
	"DIRECT ADDRESS",
	"bx"
      };

      sprintf(result, "mov %s, %s", second_reg, MOD00[rm]);
      *move = 2;
    }
    break;
    
  case(MOV_MEM_REG_DISP8):
    {
      // Index these by r/m
      static const char * MOD01[] = {
	"bx + si",
	"bx + di",
	"bp + si",
	"bp + di",
	"si",
	"di",
	"bp",
	"bx"
      };

      u8 data = buffer[2];
      sprintf(result, "mov %s, [%s + %d]", second_reg, MOD01[rm], data);
      *move = 3;
      return;
    }
    break;

  case(MOV_MEM_REG_DISP16):
    {
      // Index these by r/m
      static const char * MOD10[] = {
	"bx + si",
	"bx + di",
	"bp + si",
	"bp + di",
	"si",
	"di",
	"bp",
	"bx"
      };

      u16 data = join(buffer[2], buffer[3]); //  TODO this the right order?
      sprintf(result, "mov %s, [%s + %d]", second_reg, MOD10[rm], data);
      *move = 4;
      return;
    }
    break;
     
  case MOV_IM_REG: {
    static const u8 REG_MASK = 0x03;
    static const u8 W_MASK = 0b00001000;
    u8 w = buffer[0] & W_MASK;
      
    if(w) {
      const char* reg = w1_reg_names[(buffer[0] & REG_MASK) >> 0];
      u16 data = join(buffer[1], buffer[2]);
      sprintf(result, "mov %s, %d", reg, data);
      *move = 3;
      return;
    }
    else {
      const char* reg = w0_reg_names[(buffer[0] & REG_MASK) >> 0];
      u8 data = buffer[1];
      sprintf(result, "mov %s, %d", reg, data);
      *move = 2;
      return;
    }
  }
  default:
    assert(false);
    break;
  }
}


