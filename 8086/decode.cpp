#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "bit_ops.cpp"

enum class OpType {
  MOV,
  ADD,
  SUB,
  CMP,
};

enum class OpAct {
  MEM_REG        , // memory address to register (MOD 00)
  MEM_REG_DISP8  , //                            (MOD 01)
  MEM_REG_DISP16 , //                            (MOD 10)
  REG_REG        , // register to register       (MOD 11)
  IM_REG         , // immediate to register
  IM_REG_MEM     , // immediate to register memory
};

struct OpDesc {
  OpType type;
  OpAct  act;
};

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

OpDesc decode_operation(const u8 *buffer) {
  OpDesc result {};
  result.type = OpType::MOV;

  static const u8 im_to_reg = 0b10110000;
  static const u8 im_to_reg_mask = 0b11110000;
  if(matches(buffer[0], im_to_reg, im_to_reg_mask)) {
    result.act = OpAct::IM_REG;
    return result;
  }

  //--------------------------------------------------
  // here we assume that mod exists in the instruction
  //..................................................
  OpAct act;
  {
    static const u8 mod_mask = 0b11000000;
    static const u8 mod_shift = 6;
    u8 mod = (buffer[1] & mod_mask) >> mod_shift;
    switch(mod) {
      case 0x00:
	act = OpAct::MEM_REG;
	break;
      case 0x01:
	act = OpAct::MEM_REG_DISP8;
	break;
      case 0x02:
	act = OpAct::MEM_REG_DISP16;
	break;
      case 0x04:
      default:
	act = OpAct::REG_REG;
	break;
    }
  }
  //--------------------------------------------------

  static const u8 mov_pattern      = 0b10001000;
  static const u8 mov_pattern_mask = 0b11111100;
  if(matches(buffer[0], mov_pattern, mov_pattern_mask)) {
    result.act = act;
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
    result.act = OpAct::IM_REG_MEM;
    static const u8 mask  = 0b00111000;
    static const u8 shift = 3;
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

void write_assembly(char *out, OpType op, const char* src, const char* dest, bool flip) {
  const char* op_str;
  switch(op) {
    case OpType::MOV: op_str = "mov"; break;
    case OpType::ADD: op_str = "add"; break;
    case OpType::SUB: op_str = "sub"; break;
    case OpType::CMP: op_str = "cmp"; break;
    default: assert(false && "op string table not filled out");
  }
  
  flip
    ? sprintf(out, "%s %s, %s", op_str, dest, src)
    : sprintf(out, "%s %s, %s", op_str, src, dest);
}

void write_u8_disp_assembly(char *out, OpType op, const char* src, u8 disp, const char* dest, bool flip) {
  char src_with_disp[20];
  sprintf(src_with_disp, "[%s + %d]", src, disp);
  write_assembly(out, op, src_with_disp, dest, flip);
}

void write_u16_disp_assembly(char *out, OpType op, const char* src, u16 disp, const char* dest, bool flip) {
  char src_with_disp[20];
  sprintf(src_with_disp, "[%s + %d]", src, disp);
  write_assembly(out, op, src_with_disp, dest, flip);
}

void write_imm_u8_assembly(char *out, OpType op, u8 val, const char* dest) {
  const char* op_str;
  switch(op) {
    case OpType::MOV: op_str = "mov"; break;
    case OpType::ADD: op_str = "add"; break;
    case OpType::SUB: op_str = "sub"; break;
    case OpType::CMP: op_str = "cmp"; break;
    default: assert(false && "op string table not filled out");
  }
  sprintf(out, "%s %s, %d", op_str, dest, val);
}

void write_imm_u16_assembly(char *out, OpType op, u16 val, const char* dest) {
  const char* op_str;
  switch(op) {
    case OpType::MOV: op_str = "mov"; break;
    case OpType::ADD: op_str = "add"; break;
    case OpType::SUB: op_str = "sub"; break;
    case OpType::CMP: op_str = "cmp"; break;
    default: assert(false && "op string table not filled out");
  }
  sprintf(out, "%s %s, %d", op_str, dest, val);
}

// Fills out the result field with the assembly
void decode(const u8* buffer, u16* move, char result[]) {
  OpDesc operation = decode_operation(buffer);
  
  static const u8 REG_MASK = 0b00111000;
  static const u8 REG_SHIFT = 3;
  static const u8 D_MASK = 0b00000010;
  static const u8 RM_MASK = 0b00000111;
  static const u8 RM_SHIFT = 0;
  
  u8 reg = (buffer[1] & REG_MASK) >> REG_SHIFT;
  u8 rm_reg = (buffer[1] & RM_MASK) >> RM_SHIFT;

  // swap the destination and source?
  u8 d = buffer[0] & D_MASK;
  
  static const u8 W_MASK = 0b00000001;
  u8 w = buffer[0] & W_MASK;
  
  switch (operation.act) {

    case(OpAct::REG_REG):
      {
	const char* reg_name = get_reg_name(reg, w);
	const char* rm_reg_name = get_reg_name(rm_reg, w);
	//sprintf(result, "mov %s, %s", rm_reg_name, reg_name);
	write_assembly(result, operation.type, rm_reg_name, reg_name, d);
      
	*move = 2;
	return;
      }
      break;
    case(OpAct::MEM_REG):
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
	write_assembly(result, operation.type, this_reg, that_reg, d);
	*move = 2;
      }
      break;
    
    case(OpAct::MEM_REG_DISP8):
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
	write_u8_disp_assembly(result, operation.type, this_reg, data, that_reg, d);
	*move = 3;
	return;
      }
      break;

    case(OpAct::MEM_REG_DISP16):
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
	write_u16_disp_assembly(result, operation.type, this_reg, data, that_reg, d);
	*move = 4;
	return;
      }
      break;
     
    case OpAct::IM_REG: {
      static const u8 REG_MASK = 0b00000111;
      u8 reg = buffer[0] & REG_MASK;
    static const u8 W_MASK = 0b00001000;  
    u8 w = buffer[0] & W_MASK;
      
    if(w) {
      const char* reg_name = get_reg_name(reg, w);
      u16 data = join(buffer[1], buffer[2]);
      write_imm_u16_assembly(result, operation.type, data, reg_name);
      *move = 3;
      return;
    }
    else {
      const char* reg_name = get_reg_name(reg, w);
      u8 data = buffer[1];
      write_imm_u8_assembly(result, operation.type, data, reg_name);
      *move = 2;
      return;
    }
    }
    case OpAct::IM_REG_MEM: {
      if(w) {
	const char* reg_name = get_reg_name(reg, w);
	u16 data = join(buffer[1], buffer[2]);
	write_imm_u16_assembly(result, operation.type, data, reg_name);
	*move = 6;
	return;
      }
      else {
	const char* reg_name = get_reg_name(reg, w);
	u8 data = buffer[1];
	write_imm_u8_assembly(result, operation.type, data, reg_name);
	*move = 5;
	return;
      }
    }
    default:
      assert(false);
      break;
  }
}


