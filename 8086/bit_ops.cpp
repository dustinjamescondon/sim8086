#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef int16_t i16;
typedef uint16_t u16;
typedef unsigned char u8;

u16 join(u8 lower, u8 upper) {
  u16 padded_upper = (u16)upper;
  u16 padded_lower = (u16)lower;
  return (padded_lower) | (padded_upper << 8);
}

u8 upper(u16 data) {
  return (u8)((data & 0xFF00) >> 8);
}

u8 lower(u16 data) {
  return (u8)(data & 0x00FF);
}
