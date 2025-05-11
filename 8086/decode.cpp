#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "bit_ops.cpp"

typedef enum {
  MOV_MEM_REG        = 0x00, // memory address to register (MOD 00)
  MOV_MEM_REG_DISP8  = 0x01, //                            (MOD 01)
  MOV_MEM_REG_DISP16 = 0x02, //                            (MOD 10)
  MOV_REG_REG        = 0x03, // register to register       (MOD 11)

  MOV_IM_REG,         // immediate to register
} Operation;

const char* _w0_reg_names[] = {    
  "al",
  "cl",
  "dl",
  "bl",
  "ah",
  "ch",
  "dh",
  "bh"
};

const char* _w1_reg_names[] = {    
  "ax",
  "cx",
  "dx",
  "bx",
  "sp",
  "bp",
  "si",
  "di"
};

const char* get_reg_name(u8 reg, u8 w) {
  return w ? _w1_reg_names[reg] : _w0_reg_names[reg];
}

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
  
  return (Operation)0;
}

void write_move_assembly(char *out, const char* src, const char* dest, bool flip) {
  flip
    ? sprintf(out, "mov %s, %s", dest, src)
    : sprintf(out, "mov %s, %s", src, dest);
}

void write_u8_disp_move_assembly(char *out, const char* src, u8 disp, const char* dest, bool flip) {
  char src_with_disp[20];
  sprintf(src_with_disp, "[%s + %d]", src, disp);
  write_move_assembly(out, src_with_disp, dest, flip);
}

void write_u16_disp_move_assembly(char *out, const char* src, u16 disp, const char* dest, bool flip) {
  char src_with_disp[20];
  sprintf(src_with_disp, "[%s + %d]", src, disp);
  write_move_assembly(out, src_with_disp, dest, flip);
}

void write_imm_u8_move_assembly(char *out, u8 val, const char* dest) {
  sprintf(out, "mov %s, %d", dest, val);
}

void write_imm_u16_move_assembly(char *out, u16 val, const char* dest) {
  sprintf(out, "mov %s, %d", dest, val);
}

// Fills out the result field with the assembly
void decode(const u8* buffer, u16* move, char result[]) {
  Operation operation = decode_operation(buffer);

  static const u8 REG_MASK = 0b00111000;
  static const u8 REG_SHIFT = 3;
  static const u8 D_MASK = 0b00000010;
  static const u8 RM_MASK = 0b00000111;
  static const u8 RM_SHIFT = 0;
  
  u8 reg = (buffer[1] & REG_MASK) >> REG_SHIFT;
  u8 rm_reg = (buffer[1] & RM_MASK) >> RM_SHIFT;

  // swap the destination and source?
  u8 d = buffer[0] & D_MASK;
  
  static const u8 W_MASK = 0x01;   // 00000001
  u8 w = buffer[0] & W_MASK;
  
  switch (operation) {

  case(MOV_REG_REG):
    {
      const char* reg_name = get_reg_name(reg, w);
      const char* rm_reg_name = get_reg_name(rm_reg, w);
      //sprintf(result, "mov %s, %s", rm_reg_name, reg_name);
      write_move_assembly(result, rm_reg_name, reg_name, d);
      
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

      const char* this_reg = MOD00[rm_reg];
      const char* that_reg = get_reg_name(reg, w);
      write_move_assembly(result, this_reg, that_reg, d);
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

      const char* this_reg = MOD01[rm_reg];
      const char* that_reg = get_reg_name(reg, w);
      write_u8_disp_move_assembly(result, this_reg, data, that_reg, d);
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

      u16 data = join(buffer[2], buffer[3]);
      const char* this_reg = MOD10[rm_reg];
      const char* that_reg = get_reg_name(reg, w);
      write_u16_disp_move_assembly(result, this_reg, data, that_reg, d);
      *move = 4;
      return;
    }
    break;
     
  case MOV_IM_REG: {
    static const u8 REG_MASK = 0b00000111;
    u8 reg = buffer[0] & REG_MASK;
    static const u8 W_MASK = 0b00001000;  
    u8 w = buffer[0] & W_MASK;
      
    if(w) {
      const char* reg_name = get_reg_name(reg, w);
      u16 data = join(buffer[1], buffer[2]);
      write_imm_u16_move_assembly(result, data, reg_name);
      *move = 3;
      return;
    }
    else {
      const char* reg_name = get_reg_name(reg, w);
      u8 data = buffer[1];
      write_imm_u8_move_assembly(result, data, reg_name);
      *move = 2;
      return;
    }
  }
  default:
    assert(false);
    break;
  }
}


