#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include "bit_ops.cpp"
#include <functional>

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
  JNP,
  JNO,
  JNS,
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

const char* decode_rm(u8 rm, ModType mod, bool wide) {
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
    const char* rm_part = decode_rm(rm, mod_type, (bool)w);
    
    switch(mod_type) {
      case ModType::MEM_MODE:{
	*move = 2;
	sprintf(result.dest, "%s", rm_part);
      }
	break;
      case ModType::MEM_MODE_DISP8:{
	u8 disp = buffer[2];
	*move = 3;
	sprintf(result.dest, "[%s + %d]", rm_part, disp);
      }
	break;
      case ModType::MEM_MODE_DISP16: {
	u16 disp = join(buffer[2], buffer[3]);	
	*move = 4;
	sprintf(result.dest, "[%s + %d]", rm_part, disp);
      }
	break;
      case ModType::REG_MODE: {
	*move = 2;
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
    const char* rm_part = decode_rm(rm, mod_type, (bool)w);

    u16 data = 0;
    
    switch(mod_type) {
      case ModType::MEM_MODE:{
	u8 data_size = 0;
	const u8* data_start = buffer + 2;
	data = extract_data(data_start, w, s, &data_size);
	*move =
	  2            // the instruction
	  + data_size; // the data
	const char* size = w ? "word" : "byte";
	sprintf(result.dest, "%s %s", size, rm_part);
      }
	break;
      case ModType::MEM_MODE_DISP8:{
	u8 disp = buffer[2];
	u8 data_size = 0;
	const u8* data_start = buffer + 3;
	data = extract_data(data_start, w, s, &data_size);
       
	*move =
	  2            // the instruction
	  + 1          // the displacement
	  + data_size; // the data

	const char* size = w ? "word" : "byte";
	sprintf(result.dest, "%s [%s + %d]", size, rm_part, disp);
      }
	break;
      case ModType::MEM_MODE_DISP16: {
	u16 disp = join(buffer[2], buffer[3]);
	u8 data_size = 0;
	const u8* data_start = buffer + 4;
	data = extract_data(data_start, w, s, &data_size);
	
	*move =
	  2            // the instruction
	  + 2          // the displacement
	  + data_size; // the data
	const char* size = w ? "word" : "byte";
	sprintf(result.dest, "%s [%s + %d]", size, rm_part, disp);
      }
	break;
      case ModType::REG_MODE: {
	u8 data_size = 0;
	const u8* data_start = buffer + 2;
	data = extract_data(data_start, w, s, &data_size);
	*move =
	  2            // the instruction
	  + data_size; // the data
	sprintf(result.dest, "%s", rm_part);
      }
	break;
    }
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
    
    SrcDest src_dest = decode_src_dest_reg_mem(buffer, &result.move);
    strcpy(result.src_dest.src,  src_dest.src);
    strcpy(result.src_dest.dest, src_dest.dest);
    
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

OpDesc decode_conditional_jmp(OpType jmp, const u8* buffer) {
  OpDesc result{};
  result.type = jmp;
  result.move = 2;
  result.rel_jump = (i8)buffer[1];
  return result;
}

struct Row {
  Row(const char* head, std::function<OpDesc(const u8*)> fn) {
    this->head      = head;
    this->decode_fn = fn;
  }
  const char* head;
  std::function<OpDesc(const u8*)> decode_fn;
};

static const Row rows[] = {
  Row("1011****", decode_move_im_to_reg),
  Row("100010**", decode_move),
  Row("1*******", decode_im_add_sub_cmp),
};

struct DecodeTable {
};

OpDesc decode_operation(const u8 *buffer) {
  static const u8 mov_im_to_reg      = 0b10110000;
  static const u8 mov_im_to_reg_mask = 0b11110000;
  if(matches(buffer[0], mov_im_to_reg, mov_im_to_reg_mask)) {
    return decode_move_im_to_reg(buffer);
  }

  static const u8 mov_pattern      = 0b10001000;
  static const u8 mov_pattern_mask = 0b11111100;
  if(matches(buffer[0], mov_pattern, mov_pattern_mask)) {
    return decode_move(buffer);
  }

  static const u8 im_add_sub_cmp_pattern = 0b10000000;
  static const u8 im_add_sub_cmp_mask    = 0b10000000;
  // immediate to mem/reg
  if(matches(buffer[0], im_add_sub_cmp_pattern, im_add_sub_cmp_mask)) {
    return decode_im_add_sub_cmp(buffer);
  }

  // reg/mem to reg/mem
  static const u8 add_sub_cmp_pattern = 0b00000000;
  static const u8 add_sub_cmp_mask    = 0b11000100;
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

    result.src_dest = decode_src_dest_reg_mem(buffer, &result.move);
    return result;
  }

  static const u8 im_to_accum_add_sub_cmp_pattern = 0b00000100;
  static const u8 im_to_accum_add_sub_cmp_mask    = 0b00000100;
  if(matches(buffer[0], im_to_accum_add_sub_cmp_pattern, im_to_accum_add_sub_cmp_mask)) {
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

  static u8 jump_codes[] {
    0b01110100, // JE
    0b01111100, // JL
    0b01111110, // JLE
    0b01110010, // JB
    0b01110110, // JBE
    0b01111010, // JP
    0b01110000, // JO
    0b01111000, // JS
    0b01110101, // JNE
    0b01111101, // JNL
    0b01111111, // JNLE
    0b01110011, // JNB
    0b01110111, // JNBE
    0b01111011, // JNP
    0b01110001, // JNO
    0b01111001, // JNS
    0b11100011, // JCXZ
  };
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
    "jnp",
    "jno",
    "jns",
    "jcxz",
  };

  return op_strings[(int)op];
}

// Fills out the result field with the assembly
void decode(const u8* buffer, u16* move, char result[]) {
  OpDesc operation = decode_operation(buffer);
  const char* op_str = decode_op(operation.type);

  if(operation.type >= OpType::JE) {
    sprintf(result, "%s ");
  } else {
    sprintf(result, "%s %s, %s", op_str, operation.src_dest.dest, operation.src_dest.src);
  }
 
  *move = operation.move;
}  
