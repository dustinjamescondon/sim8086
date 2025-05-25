#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include "bit_ops.cpp"
#include <functional>

#define SIZE(ARR) sizeof(ARR) / sizeof(ARR[0])

enum class OpType {
  MOV = 0,
  ADD,
  SUB,
  CMP,
  JE,
  JL,
  JLE,
  JB,
  JBE,
  JP,
  JO,
  JS,
  JNE,
  JNL,
  JNLE,
  JNB,
  JNBE,
  JNP,
  JNO,
  JNS,
  LOOP,
  LOOPZ,
  LOOPNZ,
  JCXZ,
  COUNT
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

struct SrcDest {
  char src[50];
  char dest[50];
};

struct OpDesc {
  OpType type;
  union { 
    SrcDest src_dest;
    i8      rel_jump;
  };
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

void decode_effective_address(char* result, u8 rm, ModType mod, bool wide, const u8* buffer, u8* disp_size) {  
  // Index these by r/m
  static const char * MOD00[] = {
    "[bx + si]",
    "[bx + di]",
    "[bp + si]",
    "[bp + di]",
    "[si]",
    "[di]",
    "DIRECT ADDRESS",
    "[bx]"
  };
  // Index these by r/m
  static const char * MOD01[] = {
    "[bx + si + %d]",
    "[bx + di + %d]",
    "[bp + si + %d]",
    "[bp + di + %d]",
    "[si + %d]",
    "[di + %d]",
    "[bp + %d]",
    "[bx + %d]"
  };
  // Index these by r/m
  static const char * MOD10[] = {
    "[bx + si + %d]",
    "[bx + di + %d]",
    "[bp + si + %d]",
    "[bp + di + %d]",
    "[si + %d]",
    "[di + %d]",
    "[bp + %d]",
    "[bx + %d]"
  };

  switch(mod) {
    case ModType::MEM_MODE:{
      // the DIRECT ADDRESS case
      if(rm == 0b110){
	if(wide) {
	  u16 address = join(buffer[0], buffer[1]);
	  *disp_size = 2;
	  sprintf(result, "[%d]", address);
	} else {
	  u8 address = buffer[0];
	  *disp_size = 1;
	  sprintf(result, "[%d]", address);
	}
	return;
      }
      
      strcpy(result, MOD00[rm]);
      return;
    }
    case ModType::MEM_MODE_DISP8: {
      *disp_size = 1;
      u8 disp = buffer[0];
      sprintf(result, MOD01[rm], disp);
      return;
    }
    case ModType::MEM_MODE_DISP16: {
      *disp_size = 2;
      u16 disp = join(buffer[0], buffer[1]);
      sprintf(result, MOD10[rm], disp);
      return;
    }
    case ModType::REG_MODE:{
      *disp_size = 0;
      strcpy(result, decode_reg(rm, wide));
      return;
    }
  }

  assert(false && "");
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

// See first row of MOV
SrcDest decode_src_dest_reg_mem(const u8* buffer, u8* move) {
  // NOTE: the rest of this is common to many other operations
  static const u8 w_mask  = 0b00000001;
  u8 w = buffer[0] & w_mask;
  static const u8 d_mask  = 0b00000010;
  u8 d = buffer[0] & d_mask;
  ModType mod_type = decode_mod(buffer[1]);

  SrcDest result{};

  u8 rm_mask  = 0b00000111;
  u8 rm       = (buffer[1] & rm_mask);
  u8 reg_mask = 0b00111000;
  u8 reg      = (buffer[1] & reg_mask) >> 3;

  char rm_part[50];
  u8 disp_size = 0;
  decode_effective_address(rm_part, rm, mod_type, (bool)w, buffer + 2, &disp_size);
    
  *move =
    2            // instruction size
    + disp_size; // displacement size

  strcpy(result.dest, rm_part);
  strcpy(result.src, decode_reg(reg, (bool)w));
 
  if(d) {
    std::swap(result.dest, result.src);
  }

  return result;
}

u16 extract_data(const u8* data, bool w, bool s, u8 *data_size) {
  if(!w) {
    *data_size = 1;
    return join(data[0], 0);
  }

  if(s) {
    *data_size = 1;
    return join(data[0], 0);
  } else {
    *data_size = 2;
    return join(data[0], data[1]);
  }
}

SrcDest decode_src_dest_im_to_acc(const u8* buffer, u8* move) {
  static const u8 w_mask  = 0b00000001;
  u8 w = buffer[0] & w_mask;

  SrcDest result{};
  u16 data = w ? join(buffer[1], buffer[2]) : join(buffer[1], 0);
  sprintf(result.src, "%d", data);
  sprintf(result.dest, "%s", w ? "ax" : "al");

  *move = w ? 3 : 2;
    
  return result;
}

// See second row of ADD
SrcDest decode_src_dest_im_to_reg_mem(const u8* buffer, u8* move) {
  // NOTE: the rest of this is common to many other operations
  static const u8 w_mask  = 0b00000001;
  u8 w = buffer[0] & w_mask;
  static const u8 s_mask  = 0b00000010;
  u8 s = buffer[0] & s_mask;
    
  ModType mod_type = decode_mod(buffer[1]);

  SrcDest result{};

  u8 rm_mask  = 0b00000111;
  u8 rm       = (buffer[1] & rm_mask);
  char rm_part[50];
  u8 disp_size = 0;
  decode_effective_address(rm_part, rm, mod_type, (bool)w, buffer + 2, &disp_size);

  u16 data = 0;
  u8 data_size = 0;
  
  const u8* data_start = buffer
    + 2
    + disp_size;
  data = extract_data(data_start, w, s, &data_size);
  *move =
    2            // the instruction
    + disp_size
    + data_size;
  
  const char* size = w ? "word" : "byte";
  sprintf(result.dest, "%s %s", size, rm_part);
  sprintf(result.src, "%d", data);
    
  return result;
}

OpDesc decode_move_im_to_reg(const u8* buffer) {
  static const u8 w_mask = 0b00001000;
  u8 w = buffer[0] & w_mask;
    
  OpDesc result{};
  result.type = OpType::MOV;

  u16 data = w
    ? join(buffer[1], buffer[2])
    : join(buffer[1], 0);
  sprintf(result.src_dest.src, "%d", data);

  result.move = w ? 3 : 2;
  static const u8 reg_mask = 0b00000111;
  u8 reg = buffer[0] & reg_mask;
  strcpy(result.src_dest.dest, decode_reg(reg, w));
  return result;
}

OpDesc decode_move(const u8* buffer) {
  OpDesc result{};
  result.src_dest = decode_src_dest_reg_mem(buffer, &result.move);    
  return result;
}

static const u8 add_pattern            = 0b00000000;
static const u8 sub_pattern            = 0b00000101;
static const u8 cmp_pattern            = 0b00000111;

OpDesc decode_im_add_sub_cmp(const u8* buffer) {
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

  result.src_dest = decode_src_dest_im_to_reg_mem(buffer, &result.move);
  return result;
}

OpDesc decode_add_sub_cmp(const u8* buffer) {
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

  result.src_dest = decode_src_dest_reg_mem(buffer, &result.move);
  return result;
}

OpDesc decode_im_to_accum_add_sub_cmp(const u8* buffer) {
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
    
  result.src_dest = decode_src_dest_im_to_acc(buffer, &result.move);
  return result;
}

OpDesc decode_conditional_jmp(OpType jmp, const u8* buffer) {
  OpDesc result{};
  result.type = jmp;
  result.move = 2;
  result.rel_jump = (i8)buffer[1];
  return result;
}

struct DecodeRow {
  DecodeRow(const char* head, std::function<OpDesc(const u8*)> fn) {
    this->head_pattern = head;
    
    { // Fill in the target bit pattern and the mask
      this->mask = 0;
      this->bit_pattern = 0;
      for(int i = 0; i < 8; i++) {
	char c = head_pattern[i];
	u8 byte_pos = 7 - i;
	if(c == '1') {
	  this->bit_pattern |= 1 << byte_pos;
	  this->mask |= 1 << byte_pos;
	}
	else if(c == '0') {
	  this->mask |= 1 << byte_pos;
	}
      }
    }
    
    this->decode_fn = fn;
  }

  bool matches(u8 data) const {
    u8 masked_code = data & this->mask;
    return masked_code == bit_pattern;
  }
  
  const char* head_pattern;
  u8 bit_pattern;
  u8 mask;
  std::function<OpDesc(const u8*)> decode_fn;
};

OpDesc decode_operation(const u8 *buffer) {
  static const DecodeRow rows[] = {
    DecodeRow("1011****", decode_move_im_to_reg),
    DecodeRow("100010**", decode_move),
    DecodeRow("00***0**", decode_add_sub_cmp),
    DecodeRow("100000**", decode_im_add_sub_cmp),
    DecodeRow("00***10*", decode_im_to_accum_add_sub_cmp),
    DecodeRow("01110100", [](const u8* buffer){ return decode_conditional_jmp(OpType::JE, buffer);}), 
    DecodeRow("01111100", [](const u8* buffer){ return decode_conditional_jmp(OpType::JL, buffer);}), 
    DecodeRow("01111110", [](const u8* buffer){ return decode_conditional_jmp(OpType::JLE, buffer);}),
    DecodeRow("01110010", [](const u8* buffer){ return decode_conditional_jmp(OpType::JB, buffer);}), 
    DecodeRow("01110110", [](const u8* buffer){ return decode_conditional_jmp(OpType::JBE, buffer);}),
    DecodeRow("01111010", [](const u8* buffer){ return decode_conditional_jmp(OpType::JP, buffer);}), 
    DecodeRow("01110000", [](const u8* buffer){ return decode_conditional_jmp(OpType::JO, buffer);}), 
    DecodeRow("01111000", [](const u8* buffer){ return decode_conditional_jmp(OpType::JS, buffer);}), 
    DecodeRow("01110101", [](const u8* buffer){ return decode_conditional_jmp(OpType::JNE, buffer);}),
    DecodeRow("01111101", [](const u8* buffer){ return decode_conditional_jmp(OpType::JNL, buffer);}),
    DecodeRow("01111111", [](const u8* buffer){ return decode_conditional_jmp(OpType::JNLE, buffer);}),
    DecodeRow("01110011", [](const u8* buffer){ return decode_conditional_jmp(OpType::JNB, buffer);}), 
    DecodeRow("01110111", [](const u8* buffer){ return decode_conditional_jmp(OpType::JNBE, buffer);}),
    DecodeRow("01111011", [](const u8* buffer){ return decode_conditional_jmp(OpType::JNP, buffer);}), 
    DecodeRow("01110001", [](const u8* buffer){ return decode_conditional_jmp(OpType::JNO, buffer);}), 
    DecodeRow("01111001", [](const u8* buffer){ return decode_conditional_jmp(OpType::JNS, buffer);}), 
    DecodeRow("11100010", [](const u8* buffer){ return decode_conditional_jmp(OpType::LOOP, buffer);}),
    DecodeRow("11100001", [](const u8* buffer){ return decode_conditional_jmp(OpType::LOOPZ, buffer);}), 
    DecodeRow("11100000", [](const u8* buffer){ return decode_conditional_jmp(OpType::LOOPNZ, buffer);}),
    DecodeRow("11100011", [](const u8* buffer){ return decode_conditional_jmp(OpType::JCXZ, buffer);}),
  };

  for(size_t i = 0; i < SIZE(rows); i++) {
    if(rows[i].matches(buffer[0])) {
      return rows[i].decode_fn(buffer);
    }
  }
 
  assert(false && "missing decode");
  return OpDesc {};
}

const char* decode_op(OpType op) {
  static const char * op_strings[] {
    "mov",
    "add",
    "sub",
    "cmp",
    "je",
    "jl",
    "jle",
    "jb",
    "jbe",
    "jp",
    "jo",
    "js",
    "jne",
    "jnl",
    "jnle",
    "jnb",
    "jnbe",
    "jnp",
    "jno",
    "jns",
    "loop",
    "loopz",
    "loopnz",
    "jcxz",
  };

  return op_strings[(int)op];
}

// Fills out the result field with the assembly
void decode(const u8* buffer, u16* move, char result[]) {
  OpDesc operation = decode_operation(buffer);
  const char* op_str = decode_op(operation.type);

  if(operation.type < OpType::JE) {
    sprintf(result, "%s %s, %s", op_str, operation.src_dest.dest, operation.src_dest.src);
  } else {
    if (operation.rel_jump >= 0) {
      sprintf(result, "%s $+%d", op_str, operation.rel_jump);
    } else {
      sprintf(result, "%s $%d", op_str, operation.rel_jump);
    }
  }
 
  *move = operation.move;
}  
