#include <stdio.h>
#include <stdint.h>

#define i16 int16_t
#define u16 uint16_t
#define W_MASK 0x0100   // 00000001,00000000
#define REG_MASK 0x0038 // 00000000,00111000
#define REG_SHIFT 3

enum Name {
  AL,
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
  DI,
};

Name decode(u16 code) {
  
}

int main(int argc, char *argv[]) {
  i16 something = 1;
  u16 somethingelse = 1;
  printf("Hello, %s\n", argv[0]);
  printf("size is %d\n", sizeof(i16));
  return 0;
}
