#pragma once

#include <stdint.h>

// This thing has been written by RusJJ a.k.a. [-=KILL MAN=-]
// https://github.com/RusJJ

union CMPBits // CMP is an alias for SUBS
{
  inline static uint32_t Create(uint32_t _imm, uint32_t _regn, bool isXreg)
  {
    // isXreg - is register X or W?
    CMPBits val; val.addr = isXreg ? 0xF100001F : 0x7100001F;

    val.regn = _regn;
    val.imm = _imm;

    return val.addr;
  }
  struct
  {
      uint32_t regd : 5; // register compare with (31 if CMP, otherwise it will became SUBS)
      uint32_t regn : 5; // register compare what?
      uint32_t imm : 12; // value is in range [0; 4095]
      uint32_t pad : 10; // descriptor of instruction
  };
  uint32_t addr;
};
union MOVBits // A real MOV (not ORR or ADDS)
{
  inline static uint32_t Create(uint32_t _imm, uint32_t _reg, bool isXreg)
  {
    // isXreg - is register X or W?
    MOVBits val; val.addr = isXreg ? 0xD2800000 : 0x52800000;

    val.reg = _reg;
    val.imm = _imm;

    return val.addr;
  }
  struct
  {
      uint32_t reg : 5;
      uint32_t imm : 16;
      uint32_t pad : 11; // descriptor of instruction
  };
  uint32_t addr;
};