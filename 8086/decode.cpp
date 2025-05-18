#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include "bit_ops.cpp"

enum class OpType {
  MOV,
  ADD,
  SUB,
  CMP,
};

enum class ModType {
  NONE = 0,
  MEM_MODE        , // memory address to register (MOD 00)
  MEM_MODE_DISP8  , //                            (MOD 01)
  MEM_MODE_DISP16 , //                            (MOD 10)
  REG_MODE        , // register to register       (MOD 11)
};

enum class SourceType {
  IMM,
  REG,
  MEM
};

struct OpDesc {
  OpType type;
  char   src[50];
  char   dest[50];
  u8     move;
};

const char* decode_reg(u8 reg, bool wide) {
  static const char* _w0_reg_names[] = {    
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh"
  };

  static const char* _w1_reg_names[] = {    
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di"
  };

  return wide ? _w1_reg_names[reg] : _w0_reg_names[reg];
}

const char* decode_rm(u8 rm, ModType mod, bool wide) {
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

  switch(mod) {
    case ModType::MEM_MODE:
      return MOD00[rm];
    case ModType::MEM_MODE_DISP8:
      return MOD01[rm];
    case ModType::MEM_MODE_DISP16:
      return MOD10[rm];
    case ModType::REG_MODE:
      return decode_reg(rm, wide);
  }
  return "TODO";
}

ModType decode_mod(u8 buffer) {
  static const u8 mod_mask = 0b11000000;
  static const u8 mod_shift = 6;
  u8 mod = (buffer & mod_mask) >> mod_shift;
  switch(mod) {
    case 0x00:
      return ModType::MEM_MODE;
      break;
    case 0x01:
      return ModType::MEM_MODE_DISP8;
      break;
    case 0x02:
      return ModType::MEM_MODE_DISP16;
      break;
    case 0x03:
      return ModType::REG_MODE;
      break;
  }
  assert(false && "Couldn't match mod");
}

bool matches(const char *bit_pattern, u8 byte) {

  size_t len = strlen(bit_pattern);
  assert(len == 8);
  
  for(int i = 0; i < 8; i++) {
    u8 bit = 1 << i;
    if(bit_pattern[7 - i] == '1' && !(bit & byte)) {
      return false;
    }
    if(bit_pattern[7 - i] == '0' && (bit & byte)) {
      return false;
    }
  }

  return true;
}

bool matches(u8 code, u8 pattern, u8 mask) {
  u8 masked_code = code & mask;
  return masked_code == pattern;
}

OpDesc decode_operation(const u8 *buffer) {
  static const u8 mov_im_to_reg      = 0b10110000;
  static const u8 mov_im_to_reg_mask = 0b11110000;
  if(matches(buffer[0], mov_im_to_reg, mov_im_to_reg_mask)) {
    static const u8 w_mask = 0b00001000;
    u8 w = buffer[0] & w_mask;
    
    OpDesc result{};
    result.type = OpType::MOV;

    u16 data = w
      ? join(buffer[1], buffer[2])
      : join(buffer[1], 0);
    sprintf(result.src, "%d", data);

    result.move = w ? 3 : 2;
    static const u8 reg_mask = 0b00000111;
    u8 reg = buffer[0] & reg_mask;
    strcpy(result.dest, decode_reg(reg, w));
    return result;
  }

  static const u8 mov_pattern      = 0b10001000;
  static const u8 mov_pattern_mask = 0b11111100;
  if(matches(buffer[0], mov_pattern, mov_pattern_mask)) {
    // NOTE: the rest of this is common to many other operations
    static const u8 w_mask  = 0b00000001;
    u8 w = buffer[0] & w_mask;
    static const u8 d_mask  = 0b00000010;
    u8 d = buffer[0] & d_mask;
    u8 mod = (buffer[1] & 0b11000000) >> 6;
    ModType mod_type = decode_mod(buffer[1]);

    OpDesc result{};
    result.type = OpType::MOV;

    u8 rm_mask  = 0b00000111;
    u8 rm       = (buffer[1] & rm_mask);
    u8 reg_mask = 0b00111000;
    u8 reg      = (buffer[1] & reg_mask) >> 3;
    const char* rm_part = decode_rm(rm, mod_type, (bool)w);
    
    switch(mod_type) {
      case ModType::MEM_MODE:{
	result.move = 2;
	sprintf(result.dest, "%s", rm_part);
      }
	break;
      case ModType::MEM_MODE_DISP8:{
	u8 disp = buffer[2];
	result.move = 3;
	sprintf(result.dest, "[%s + %d]", rm_part, disp);
      }
	break;
      case ModType::MEM_MODE_DISP16: {
	u16 disp = join(buffer[2], buffer[3]);	
	result.move = 4;
	sprintf(result.dest, "[%s + %d]", rm_part, disp);
      }
	break;
      case ModType::REG_MODE: {
	result.move = 2;
	sprintf(result.dest, "%s", rm_part);
      }
	break;
    }
   
    strcpy(result.src, decode_reg(reg, (bool)w));
 
    if(d) {
      std::swap(result.dest, result.src);
    }
   
    return result;
  }

  static const u8 im_add_sub_cmp_pattern = 0b10000000;
  static const u8 im_add_sub_cmp_mask    = 0b10000000;
  
  static const u8 add_pattern            = 0b00000000;
  static const u8 sub_pattern            = 0b00000101;
  static const u8 cmp_pattern            = 0b00000111;

  // immediate to mem/reg
  if(matches(buffer[0], im_add_sub_cmp_pattern, im_add_sub_cmp_mask)) {    
    static const u8 mask  = 0b00111000;
    static const u8 shift = 3;
    OpDesc result{};
    u8 op = (buffer[1] & mask) >> shift;
    switch(op) {
      case add_pattern:
	result.type = OpType::ADD;
	break;
      case sub_pattern:
	result.type = OpType::SUB;
	break;
      case cmp_pattern:
	result.type = OpType::CMP;
	break;
      default:
	assert(false && "immediate add/sub/cmp pattern not matched");
    }
    return result;
  }

  // reg/mem to reg/mem
  static const u8 add_sub_cmp_pattern = 0b00000000;
  static const u8 add_sub_cmp_mask    = 0b11000000;
  if(matches(buffer[0], add_sub_cmp_pattern, add_sub_cmp_mask)) {
    static const u8 mask  = 0b00111000;
    static const u8 shift = 3;

    OpDesc result {};
    u8 op = (buffer[0] & mask) >> shift;
    switch(op) {
      case add_pattern:
	result.type = OpType::ADD;
	break;
      case sub_pattern:
	result.type = OpType::SUB;
	break;
      case cmp_pattern:
	result.type = OpType::CMP;
	break;
      default:
	assert(false && "add/sub/cmp pattern not matched");
    }
    return result;
  }

  assert(false && "missing decode");
  return OpDesc {};
}

const char* decode_op(OpType op) {
  switch(op) {
    case OpType::MOV: return "mov"; break;
    case OpType::ADD: return "add"; break;
    case OpType::SUB: return "sub"; break;
    case OpType::CMP: return "cmp"; break;
    default: assert(false && "op string table not filled out");
  }
}

// Fills out the result field with the assembly
void decode(const u8* buffer, u16* move, char result[]) {
  OpDesc operation = decode_operation(buffer);
  const char* op_str = decode_op(operation.type);
  sprintf(result, "%s %s, %s", op_str, operation.dest, operation.src);
  *move = operation.move;
}  
