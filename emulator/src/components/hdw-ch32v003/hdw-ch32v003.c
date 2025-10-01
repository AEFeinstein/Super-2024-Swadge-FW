/**
 This is a system for emulating a ch32v003 by using mini-rv32ima and adding the -C extension.
 Note that this does not support the compressed byte/half-word writing that the 003 supports.
 Also, the only hardware this supports is a "faked" dma engine output where the LED data goes.
 This is accomplished by just directly reading it out of DMA1_Channel5->MADDR.

 In general this is a costly operation to run, to emulate the processor, but, it runs in its
 own thread.
 */

//==============================================================================
// Includes
//==============================================================================

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_generic.h"

#include "cnfs.h"

#include "CNFG.h"

//==============================================================================
// Functions being stubbed.
//==============================================================================

int initCh32v003(int swdio_pin);
int ch32v003WriteMemory(const uint8_t* binary, uint32_t length, uint32_t address);
int ch32v003ReadMemory(uint8_t* binary, uint32_t length, uint32_t address);
int ch32v003GetReg(int regno, uint32_t* value);
int ch32v003SetReg(int regno, uint32_t regValue);
void ch32v003CheckTerminal(void);
void ch32v003Teardown(void);
int ch32v003Resume(void);
int ch32v003WriteFlash(const uint8_t* buf, int sz);
int ch32v003WriteBitmapAsset(int slot, int asset_idx);
int ch32v003WriteBitmap(int slot, const uint8_t pixels[6][12]);
int ch32v003SelectBitmap(int slot);

#define CH32V003_MAX_IMAGE_SLOTS 20

// For functions in this code.
uint32_t GetSTK(void);

//==============================================================================
// mini-rv32ima augmentations.
//==============================================================================

static inline void* ch32v003threadFn(void* v);
static inline uint32_t MINIRV32_LOAD4s(uint32_t ofs, uint32_t* rval, uint32_t* trap);
static inline uint16_t MINIRV32_LOAD2s(uint32_t ofs, uint32_t* rval, uint32_t* trap);
static inline uint8_t MINIRV32_LOAD1s(uint32_t ofs, uint32_t* rval, uint32_t* trap);
static inline int16_t MINIRV32_LOAD2_SIGNEDs(uint32_t ofs, uint32_t* rval, uint32_t* trap);
static inline int8_t MINIRV32_LOAD1_SIGNEDs(uint32_t ofs, uint32_t* rval, uint32_t* trap);

#define RAM_SIZE   2048
#define FLASH_SIZE 16384

uint8_t ch32v003flash[FLASH_SIZE];
uint8_t ch32v003ram[RAM_SIZE];
uint32_t ch32v003InternalLEDSets;
volatile int ch32v003runMode;
volatile int ch32v003quitMode;
struct MiniRV32IMAState ch32v003state;

static int CHPLoad(uint32_t address, uint32_t* regret, int size);
static int CHPStore(uint32_t address, uint32_t regret, int size);

#define MINI_RV32_RAM_SIZE        (0x20000000 + RAM_SIZE)
#define MINIRV32_RAM_IMAGE_OFFSET 0x00000000
#define MINIRV32WARN(x...)        fprintf(stderr, x)
#define MINIRV32_MMIO_RANGE(n)    ((0x40000000 <= (n) && (n) < 0x50000000)) || (0xe0000000 <= (n) && (n) < 0xe0004000)

// These should not be accessable.
#define MINIRV32_HANDLE_MEM_STORE_CONTROL(addy, val) CHPStore(addy, val, 4);
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL(addy, val)  CHPLoad(addy, &val, 4);

#define RAMOFS 0x20000000

#define MINIRV32_CUSTOM_MEMORY_BUS

#define MINIRV32_STORE4(ofs, val)                          \
    if (ofs < FLASH_SIZE - 3)                              \
    {                                                      \
        *(uint32_t*)(ch32v003flash + ofs) = val;           \
    }                                                      \
    else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 3) \
    {                                                      \
        *(uint32_t*)(ch32v003ram + ofs - RAMOFS) = val;    \
    }                                                      \
    else                                                   \
    {                                                      \
        if (CHPStore(ofs, val, 4))                         \
        {                                                  \
            trap = (7 + 1);                                \
            rval = ofs;                                    \
        }                                                  \
    }
#define MINIRV32_STORE2(ofs, val)                          \
    if (ofs < FLASH_SIZE - 1)                              \
    {                                                      \
        *(uint16_t*)(ch32v003flash + ofs) = val;           \
    }                                                      \
    else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 1) \
    {                                                      \
        *(uint16_t*)(ch32v003ram + ofs - RAMOFS) = val;    \
    }                                                      \
    else                                                   \
    {                                                      \
        if (CHPStore(ofs, val, 2))                         \
        {                                                  \
            trap = (7 + 1);                                \
            rval = ofs;                                    \
        }                                                  \
    }
#define MINIRV32_STORE1(ofs, val)                          \
    if (ofs < FLASH_SIZE - 0)                              \
    {                                                      \
        *(uint8_t*)(ch32v003flash + ofs) = val;            \
    }                                                      \
    else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 0) \
    {                                                      \
        *(uint8_t*)(ch32v003ram + ofs - RAMOFS) = val;     \
    }                                                      \
    else                                                   \
    {                                                      \
        if (CHPStore(ofs, val, 1))                         \
        {                                                  \
            trap = (7 + 1);                                \
            rval = ofs;                                    \
        }                                                  \
    }
static inline uint32_t MINIRV32_LOAD4s(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
    uint32_t tmp;
    if (ofs < FLASH_SIZE - 3)
    {
        tmp = *(uint32_t*)(ch32v003flash + ofs);
    }
    else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 3)
    {
        tmp = *(uint32_t*)(ch32v003ram + ofs - RAMOFS);
    }
    else
    {
        if (CHPLoad(ofs, &tmp, 4))
        {
            *trap = (7 + 1);
            *rval = ofs;
        }
    }
    return tmp;
}
static inline uint16_t MINIRV32_LOAD2s(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
    uint16_t tmp;
    if (ofs < FLASH_SIZE - 1)
    {
        tmp = *(uint16_t*)(ch32v003flash + ofs);
    }
    else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 1)
    {
        tmp = *(uint16_t*)(ch32v003ram + ofs - RAMOFS);
    }
    else
    {
        if (CHPLoad(ofs, (uint32_t*)&tmp, 2))
        {
            *trap = (7 + 1);
            *rval = ofs;
        }
    }
    return tmp;
}
static inline uint8_t MINIRV32_LOAD1s(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
    uint8_t tmp;
    if (ofs < FLASH_SIZE - 0)
    {
        tmp = *(uint8_t*)(ch32v003flash + ofs);
    }
    else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 0)
    {
        tmp = *(uint8_t*)(ch32v003ram + ofs - RAMOFS);
    }
    else
    {
        if (CHPLoad(ofs, (uint32_t*)&tmp, 1))
        {
            *trap = (7 + 1);
            *rval = ofs;
        }
    }
    return tmp;
}
static inline int16_t MINIRV32_LOAD2_SIGNEDs(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
    int16_t tmp;
    if (ofs < FLASH_SIZE - 1)
    {
        tmp = *(int16_t*)(ch32v003flash + ofs);
    }
    else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 1)
    {
        tmp = *(int16_t*)(ch32v003ram + ofs - RAMOFS);
    }
    else
    {
        if (CHPLoad(ofs, (uint32_t*)&tmp, -2))
        {
            *trap = (7 + 1);
            *rval = ofs;
        }
    }
    return tmp;
}
static inline int8_t MINIRV32_LOAD1_SIGNEDs(uint32_t ofs, uint32_t* rval, uint32_t* trap)
{
    int8_t tmp;
    if (ofs < FLASH_SIZE - 0)
    {
        tmp = *(int8_t*)(ch32v003flash + ofs);
    }
    else if (ofs >= RAMOFS && ofs < RAMOFS + RAM_SIZE - 0)
    {
        tmp = *(int8_t*)(ch32v003ram + ofs - RAMOFS);
    }
    else
    {
        if (CHPLoad(ofs, (uint32_t*)&tmp, -1))
        {
            *trap = (7 + 1);
            *rval = ofs;
        }
    }
    return tmp;
}

#define MINIRV32_LOAD4(ofs)        MINIRV32_LOAD4s(ofs, &rval, &trap)
#define MINIRV32_LOAD2(ofs)        MINIRV32_LOAD2s(ofs, &rval, &trap)
#define MINIRV32_LOAD1(ofs)        MINIRV32_LOAD1s(ofs, &rval, &trap)
#define MINIRV32_LOAD2_SIGNED(ofs) MINIRV32_LOAD2_SIGNEDs(ofs, &rval, &trap)
#define MINIRV32_LOAD1_SIGNED(ofs) MINIRV32_LOAD1_SIGNEDs(ofs, &rval, &trap)

#define MINIRV32_HANDLE_TRAP_PC                                     \
    switch (CSR(mtvec) & 3)                                         \
    {                                                               \
        case 2:                                                     \
        case 0:                                                     \
            pc = (CSR(mtvec) - 4);                                  \
            break;                                                  \
        case 1:                                                     \
            pc = (((CSR(mtvec) & ~3) + 4 * CSR(mcause)) - 4);       \
            break;                                                  \
        case 3:                                                     \
            pc = ((MINIRV32_LOAD4((CSR(mtvec) & ~3) + 4 * 3)) - 4); \
            break;                                                  \
    }

#define MINIRV32_ALIGNMENT 1

#define MINIRV32_IMPLEMENTATION

// https://riscv.github.io/riscv-isa-manual/snapshot/unprivileged/#_compressed_instruction_formats about 1/3 the way
// down. https://www.cs.sfu.ca/~ashriram/Courses/CS295/assets/notebooks/RISCV/RISCV_CARD.pdf
// This adds -C extension support to mini-rv32ima.

#define MINIRV32_HANDLE_OTHER_OPCODE                                                                                   \
    default:                                                                                                           \
    {                                                                                                                  \
        int cimm         = ((((ir >> 2) & 0x1f)) | (((ir >> 12) & 1) << 5));                                           \
        uint32_t cimmext = (cimm & 0x20) ? (cimm | 0xffffffc0) : cimm;                                                 \
        switch (ir & 3)                                                                                                \
        {                                                                                                              \
            case 0b00:                                                                                                 \
            {                                                                                                          \
                ir &= 0xffff;                                                                                          \
                pc -= 2;                                                                                               \
                uint32_t uimm = (((ir >> 5) & 1) << 6) | (((ir >> 6) & 1) << 2) | (((ir >> 10) & 7) << 3);             \
                switch (ir >> 13)                                                                                      \
                {                                                                                                      \
                    case 0b000: /*c.addi4spn ADD Imm * 4 + SP TODO*/                                                   \
                        cimm = (((ir >> 5) & 1) << 3) | (((ir >> 6) & 1) << 2) | (((ir >> 7) & 0xf) << 6)              \
                               | (((ir >> 11) & 3) << 4);                                                              \
                        cimmext = (cimm & 0x200) ? (cimm | 0xfffffc00) : cimm;                                         \
                        rdid    = ((ir >> 2) & 7) + 8;                                                                 \
                        /*printf( "c.addi4spn %08x %d / %d\n", REG( 2 ), cimmext, rdid );*/                            \
                        rval = REG(2) + cimmext;                                                                       \
                        break;                                                                                         \
                    case 0b010: /*c.lw*/                                                                               \
                        /*printf( "L %08x -> %08x/%08x\n", pc, REG(((ir>>7)&7)+8), uimm );*/                           \
                        rval = MINIRV32_LOAD4(REG(((ir >> 7) & 7) + 8) + uimm);                                        \
                        rdid = ((ir >> 2) & 7) + 8;                                                                    \
                        break;                                                                                         \
                    case 0b110: /*c.sw*/                                                                               \
                        /*printf( "S %08x -> %08x/%08x\n", pc, REG(((ir>>7)&7)+8), uimm );*/                           \
                        MINIRV32_STORE4(REG(((ir >> 7) & 7) + 8) + uimm, REG(((ir >> 2) & 7) + 8));                    \
                        rdid = 0;                                                                                      \
                        break;                                                                                         \
                    default:                                                                                           \
                        trap = (2 + 1);                                                                                \
                        break;                                                                                         \
                }                                                                                                      \
                break;                                                                                                 \
            }                                                                                                          \
            case 0b01:                                                                                                 \
                ir &= 0xffff;                                                                                          \
                pc -= 2;                                                                                               \
                switch (ir >> 13)                                                                                      \
                {                                                                                                      \
                    case 0b000: /*c.addi*/                                                                             \
                        rval = REG(rdid) + cimmext;                                                                    \
                        break;                                                                                         \
                    case 0b010: /*c.li*/                                                                               \
                        rval = cimmext;                                                                                \
                        break;                                                                                         \
                    case 0b011: /*c.lui / ADDI16SP*/                                                                   \
                        switch (((ir >> 7) & 0x1f))                                                                    \
                        {                                                                                              \
                            case 0:                                                                                    \
                                break;                                                                                 \
                            case 2: /*c.addi16sp TODO: Check me.*/                                                     \
                            {                                                                                          \
                                cimm = (((ir >> 12) & 1) << 9) | (((ir >> 2) & 1) << 5) | (((ir >> 5) & 1) << 6)       \
                                       | (((ir >> 6) & 1) << 4) | (((ir >> 3) & 3) << 7);                              \
                                cimmext = (cimm & 0x200) ? (cimm | 0xfffffc00) : cimm;                                 \
                                /*printf( "c.addi16sp -> %08x -> %08x\n",  REG(2), cimmext );*/                        \
                                rval = REG(2) + cimmext;                                                               \
                                break;                                                                                 \
                            }                                                                                          \
                            default: /*c.lui*/                                                                         \
                                rval = cimm << 12;                                                                     \
                                break;                                                                                 \
                        };                                                                                             \
                        break;                                                                                         \
                    case 0b100: /* MISC-ALU */                                                                         \
                        rdid = (rdid & 7) + 8;                                                                         \
                        switch ((ir >> 10) & 3)                                                                        \
                        {                                                                                              \
                            case 0: /*c.srli*/                                                                         \
                                rval = REG(rdid) >> cimm;                                                              \
                                break;                                                                                 \
                            case 1: /*c.srai*/                                                                         \
                                rval = ((int32_t)REG(rdid)) >> cimm;                                                   \
                                break;                                                                                 \
                            case 2: /*c.andi*/                                                                         \
                                rval = REG(rdid) & cimm;                                                               \
                                break;                                                                                 \
                            case 3: /*c.other*/                                                                        \
                            {                                                                                          \
                                uint32_t rs2 = REG((cimm & 7) + 8);                                                    \
                                switch (cimm >> 3)                                                                     \
                                {                                                                                      \
                                    case 0: /* c.sub */                                                                \
                                        rval = REG(rdid) - (rs2);                                                      \
                                        break;                                                                         \
                                    case 1: /* c.xor */                                                                \
                                        rval = REG(rdid) ^ (rs2);                                                      \
                                        break;                                                                         \
                                    case 2: /* c.or  */                                                                \
                                        rval = REG(rdid) | (rs2);                                                      \
                                        break;                                                                         \
                                    case 3: /* c.and */                                                                \
                                        rval = REG(rdid) & (rs2);                                                      \
                                        break;                                                                         \
                                    default: /* res   */                                                               \
                                        trap = (2 + 1);                                                                \
                                        break;                                                                         \
                                }                                                                                      \
                            }                                                                                          \
                        }                                                                                              \
                        break;                                                                                         \
                    case 0b001: /*c.jal + NOTE: Is c.addiw on RV64*/                                                   \
                    case 0b101: /*c.j*/                                                                                \
                    {                                                                                                  \
                        uint32_t limm = (((ir >> 2) & 0x1) << 5) | (((ir >> 3) & 0x7) << 1) | (((ir >> 6) & 0x1) << 7) \
                                        | (((ir >> 7) & 0x1) << 6) | (((ir >> 8) & 0x1) << 10)                         \
                                        | (((ir >> 9) & 0x3) << 8) | (((ir >> 11) & 0x1) << 4)                         \
                                        | (((ir >> 12) & 1) << 11);                                                    \
                        if (limm & 0x800)                                                                              \
                            limm |= 0xfffff000;                                                                        \
                        if ((ir >> 13) == 0b001)                                                                       \
                        {                                                                                              \
                            rdid = 1;                                                                                  \
                            rval = pc + 4;                                                                             \
                            /*printf( "jal r1 = %08x\n", rval );*/                                                     \
                        }                                                                                              \
                        else                                                                                           \
                            rdid = 0;                                                                                  \
                        pc = pc + limm - 2;                                                                            \
                    }                                                                                                  \
                    break;                                                                                             \
                    case 0b110: /*c.beqz*/                                                                             \
                    case 0b111: /*c.bnez*/                                                                             \
                    {                                                                                                  \
                        rdid          = 0;                                                                             \
                        uint32_t limm = (((ir >> 2) & 0x1) << 5) | (((ir >> 3) & 0x3) << 1)                            \
                                        | (((ir >> 10) & 0x3) << 3) | (((ir >> 5) & 0x3) << 6)                         \
                                        | (((ir >> 12) & 0x1) << 8);                                                   \
                        if (limm & 0x100)                                                                              \
                            limm |= 0xffffff00;                                                                        \
                        if ((ir >> 13) == 0b110)                                                                       \
                        {                                                                                              \
                            if (REG(((ir >> 7) & 0x7) + 8) == 0)                                                       \
                            {                                                                                          \
                                pc = pc + limm - 2;                                                                    \
                            }                                                                                          \
                        }                                                                                              \
                        else                                                                                           \
                        {                                                                                              \
                            if (REG(((ir >> 7) & 0x7) + 8) != 0)                                                       \
                            {                                                                                          \
                                pc = pc + limm - 2;                                                                    \
                            }                                                                                          \
                        }                                                                                              \
                        break;                                                                                         \
                    }                                                                                                  \
                    default:                                                                                           \
                        trap = (2 + 1);                                                                                \
                        break;                                                                                         \
                }                                                                                                      \
                break;                                                                                                 \
            case 0b10:                                                                                                 \
                ir &= 0xffff;                                                                                          \
                pc -= 2;                                                                                               \
                switch (ir >> 13)                                                                                      \
                {                                                                                                      \
                    case 0b000: /*c.slli*/                                                                             \
                        rval = REG(rdid) << cimm;                                                                      \
                        break;                                                                                         \
                    case 0b100: /*c.mv / c.add / c.jr (/c.ret) / c.jalr */                                             \
                        if ((ir >> 12) & 1)                                                                            \
                        {                                                                                              \
                            if ((((ir >> 2) & 0x1f) != 0) && rdid != 0)                                                \
                            {                                                                                          \
                                /*printf( "Adding r %d / %d\n", (ir>>2)&0x1f, rdid );*/                                \
                                rval = REG((ir >> 2) & 0x1f) + REG(rdid); /* c.add */                                  \
                            }                                                                                          \
                            else if (rdid != 0)                                                                        \
                            {                                                                                          \
                                rdid = 1; /* c.jalr */                                                                 \
                                rval = pc + 4;                                                                         \
                                pc   = REG(rdid) - 4; /*c.jr*/                                                         \
                            }                                                                                          \
                            else                                                                                       \
                            {                                                                                          \
                                trap = (3 + 1);                                                                        \
                                break; /* EBREAK 3 = "Breakpoint" */                                                   \
                            }                                                                                          \
                        }                                                                                              \
                        else                                                                                           \
                        {                                                                                              \
                            if ((((ir >> 2) & 0x1f) != 0) && rdid != 0)                                                \
                            {                                                                                          \
                                rval = REG((ir >> 2) & 0x1f);                                                          \
                            }                                                                                          \
                            else if (rdid != 0)                                                                        \
                            {                                                                                          \
                                pc   = REG(rdid) - 4;                                                                  \
                                rdid = 0; /* c.jr */                                                                   \
                            }                                                                                          \
                            else                                                                                       \
                            {                                                                                          \
                                /* illegal opcode */                                                                   \
                                trap = (2 + 1);                                                                        \
                                break;                                                                                 \
                            }                                                                                          \
                        }                                                                                              \
                        break;                                                                                         \
                    case 0b110: /*c.swsp / c.sw(SP)*/                                                                  \
                        rdid = 0;                                                                                      \
                        /*printf( "C.SWSP gp=%08x -> REG(2)=%08x + %08x <<< %08x\n", REG(3), REG(2), (((ir>>7)&3)<<6)  \
                         * + (((ir>>9)&0xf)<<2), REG(((ir>>2)&0x1f)) ); */                                             \
                        MINIRV32_STORE4(REG(2) + (((ir >> 7) & 3) << 6) + (((ir >> 9) & 0xf) << 2),                    \
                                        REG(((ir >> 2) & 0x1f)));                                                      \
                        break;                                                                                         \
                    case 0b010: /*c.lwsp / c.lw(SP)*/                                                                  \
                        /*printf( "C.LWSP gp=%08x -> REG(2)=%08x + %08x <<< %08x\n", REG(3), REG(2), (((ir>>2)&3)<<6)  \
                         * + (((ir>>4)&0x7)<<2) + (((ir>>12)&1)<<5), REG(((ir>>2)&0x1f)) );*/                          \
                        rval = MINIRV32_LOAD4(REG(2) + (((ir >> 2) & 3) << 6) + (((ir >> 4) & 0x7) << 2)               \
                                              + (((ir >> 12) & 1) << 5));                                              \
                        rdid = ((ir >> 7) & 0x1f);                                                                     \
                        break;                                                                                         \
                    default:                                                                                           \
                        trap = (2 + 1);                                                                                \
                        break;                                                                                         \
                }                                                                                                      \
                break;                                                                                                 \
            default:                                                                                                   \
                trap = (2 + 1);                                                                                        \
                break;                                                                                                 \
        }                                                                                                              \
    }

#include "mini-rv32ima.h"

//==============================================================================
// Emulator assistance functions.
//==============================================================================

static uint32_t STK_CTLR;
static uint32_t STK_ZERO;
static uint32_t DMDATA[2];

uint32_t GetSTK()
{
    double dt = OGGetAbsoluteTime();
    if (STK_CTLR & 2)
        dt *= 48000000;
    else
        dt *= 6000000;
    return ((uint32_t)dt) - STK_ZERO;
}

static int CHPLoad(uint32_t address, uint32_t* regret, int size)
{
    if (address == 0x1ffff800 || address == 0x1ffff802)
    {
        // Doing weird option byte operations.
        *regret = 0;
        return 0;
    }

    if (size != 4)
    {
        printf("Misaligned system load %08x\n", address);
        return -1;
    }

    if (address == 0xe000f000)
        *regret = STK_CTLR;
    else if (address == 0xe00000f4 || address == 0xe00000f8)
        *regret = 0; // Tell DMDATA0/1 that we are free to printf.
    else if (address == 0xe000f008)
        *regret = GetSTK();
    else if (address == 0x40022000)
    {
        *regret = 0;
    } // FLASH->ACTLR
    else if (address == 0x40021000)
    {
        *regret = 3 | (1 << 25);
    } // R32_RCC, lie say clocks are fine.
    else if (address == 0x40021004)
    {
        *regret = 0x8;
    } // R32_RCC, lie and say PLL.
    else if (address == 0x40021008)
    {
        *regret = 0;
    } // R32_RCC
    else if (address == 0x4002100c)
    {
        *regret = 0;
    } // R32_RCC
    else if (address == 0x40021010)
    {
        *regret = 0;
    } // R32_RCC
    else if (address == 0x40021014)
    {
        *regret = 0;
    } // R32_RCC
    else if (address == 0x40021018)
    {
        *regret = 0;
    } // R32_RCC
    else if (address == 0x4002101C)
    {
        *regret = 0;
    } // R32_RCC
    else if (address >= 0x40000000 && address < 0x50000000)
    {
        // printf( "Unknown hardware read %08x\n", address ); *regret = 0;
    }
    else
    {
        printf("Load fail at %08x\n", address);
        return -1;
    }
    return 0;
}

static int CHPStore(uint32_t address, uint32_t regset, int size)
{
    if (address == 0x1ffff800 || address == 0x1ffff802)
    {
        // Doing weird option byte operations.
        return 0;
    }

    // Tricky: when doing write (but only in the emulator) it will do size 1 writes.  This is not true for the real
    // hardware.
    if ((address & 0xfffffffc) == 0x40020064 && size == 1)
    {
        // This code does partial writes.
        int ofs                 = address & 3;
        ch32v003InternalLEDSets = (ch32v003InternalLEDSets & (~(0xff << (ofs * 8)))) | (regset << (ofs * 8));
        return 0;
    }

    if (size != 4)
    {
        printf("Misaligned system store %08x\n", address);
        return -1;
    }
    if (address == 0xe000f000)
        STK_CTLR = regset;
    else if (address == 0xe00000f4 || address == 0xe00000f8)
    {
        if (address == 0xe00000f4)
            DMDATA[0] = regset;
        else
            DMDATA[1] = regset;

        if (DMDATA[0] & 0x80)
        {
            int chars = (DMDATA[0] & 0xf) - 4;
            int i;
            printf("PRINTF FROM CH32V003: [%08x %08x]\n", DMDATA[0], DMDATA[1]);
            for (i = 0; i < chars; i++)
            {
                printf("%c", ((uint8_t*)DMDATA)[i + 1]);
            }
            DMDATA[0] = 0;
        }
    }
    else if (address == 0x40022000)
    {
    } // FLASH->ACTLR
    else if (address == 0x40021000)
    {
    } // R32_RCC_CFGR0
    else if (address == 0x40021004)
    {
    } // R32_RCC_INTR
    else if (address == 0x40021008)
    {
    } // R32_RCC_APB2PRSTR
    else if (address == 0x4002100c)
    {
    } // R32_RCC_APB1PRSTR
    else if (address == 0x40021010)
    {
    } // R32_RCC_AHBPCENR
    else if (address == 0x40021014)
    {
    } // R32_RCC_APB2PCENR
    else if (address == 0x40021018)
    {
    } // R32_RCC_APB1PCENR
    else if (address == 0x4002101C)
    {
    } // DMA1_Channel5->MADDR
    else if (address == 0x40020064)
    {
        ch32v003InternalLEDSets = regset;
    }
    else if (address >= 0x40000000 && address < 0x50000000)
    {
        // printf( "Unknown hardware write %08x = %08x\n", address, regset );
    }
    else
    {
        printf("Store fail at %08x\n", address);
        return -1;
    }
    return 0;
}

static void ResetPeripherals()
{
    STK_ZERO += GetSTK();
    STK_CTLR = 0;
}

//==============================================================================
// Main Emulation Thread
//==============================================================================

og_thread_t ch32v003thread;

static void* ch32v003threadFn(void* v)
{
    memset(&ch32v003state, 0, sizeof(ch32v003state));
    ch32v003runMode = 0;

    double dLast = OGGetAbsoluteTime();
    while (ch32v003quitMode == 0)
    {
        double dNow  = OGGetAbsoluteTime();
        uint32_t tus = (dNow - dLast) * 1000000;
        if (ch32v003runMode)
        {
            /*int r = */ MiniRV32IMAStep(&ch32v003state, 0, 0, tus, 24 * tus);
            // printf( "STEP: %d\n", r );
        }
        OGUSleep(100);

        // printf( "%08x %08x %d\n", ch32v003state.pc, ch32v003state.mtvec, ch32v003runMode );

        dLast = dNow;
    }
    return 0;
}

//==============================================================================
// Main swadge emulator functionality.
//==============================================================================

int initCh32v003(int swdio_pin)
{
    ch32v003thread = OGCreateThread(ch32v003threadFn, 0);
    return 0;
}

int ch32v003WriteMemory(const uint8_t* binary, uint32_t length, uint32_t address)
{
    uint32_t rval = 0, trap = 0;
    ch32v003runMode = 0;
    int i;
    for (i = 0; i < length; i++)
        MINIRV32_STORE1(address + i, binary[i]);

    return (trap || rval) ? -1 : 0;
}

int ch32v003ReadMemory(uint8_t* binary, uint32_t length, uint32_t address)
{
    uint32_t rval = 0, trap = 0;
    ch32v003runMode = 0;
    int i;
    for (i = 0; i < length; i++)
        binary[i] = MINIRV32_LOAD4(address + i);

    return (trap || rval) ? -1 : 0;
}

int ch32v003GetReg(int regno, uint32_t* value)
{
    // TODO: Make this work with DMSTATUS
    if (regno == 4)
        *value = DMDATA[0];
    if (regno == 5)
        *value = DMDATA[1];
    return 0;
}

int ch32v003SetReg(int regno, uint32_t regValue)
{
    // TODO: Actually make this work with DMCONTROL
    if (regno == 4)
        DMDATA[0] = regValue;
    if (regno == 5)
        DMDATA[1] = regValue;
    return 0;
}

void ch32v003CheckTerminal()
{
    // Discuss: Is this needed/useful?  We already detect and printf in the emulationc ode above.
}

void ch32v003Teardown()
{
    ch32v003quitMode = true;
    OGJoinThread(ch32v003thread);
}

int ch32v003Resume()
{
    ResetPeripherals();
    memset(&ch32v003state, 0, sizeof(ch32v003state));
    ch32v003runMode = 1;
    return 0;
}

int ch32v003WriteFlash(const uint8_t* buf, int sz)
{
    return ch32v003WriteMemory(buf, sz, 0);
}

static const uint16_t Coordmap[] = {
    0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0xffff, 0xffff, 0x0002, 0x0102, 0x0202, 0x0302, 0x0402, 0x0502,
    0xffff, 0xffff, 0x0001, 0x0101, 0x0201, 0x0301, 0x0401, 0x0501, 0xffff, 0xffff, 0x0008, 0x0108, 0x0208, 0x0308,
    0x0408, 0x0508, 0xffff, 0xffff, 0x0007, 0x0107, 0x0207, 0x0307, 0x0407, 0x0507, 0xffff, 0xffff, 0x0006, 0x0106,
    0x0206, 0x0306, 0x0406, 0x0506, 0xffff, 0xffff, 0x0005, 0x0205, 0x0405, 0x0605, 0x0600, 0x0700, 0xffff, 0xffff,
    0x0004, 0x0204, 0x0404, 0x0604, 0x0602, 0x0702, 0xffff, 0xffff, 0x0003, 0x0203, 0x0403, 0x0603, 0x0601, 0x0701,
    0xffff, 0xffff, 0x0105, 0x0305, 0x0505, 0x0705, 0x0608, 0x0708, 0xffff, 0xffff, 0x0104, 0x0304, 0x0504, 0x0704,
    0x0607, 0x0707, 0xffff, 0xffff, 0x0103, 0x0303, 0x0503, 0x0703, 0x0606, 0x0706, 0xffff, 0xffff,
};

void ch32v003EmuDraw(int offX, int offY, int window_w, int window_h)
{
    // We use the corresponding coordmap from swadge_matrix.h, so we can backtrack the steps
    // the source material takes to output the LEDs
    uint32_t ils = ch32v003InternalLEDSets;

    // Make sure the DMA pointer is valid.  Otherwise, don't render the LEDs.
    if (ils < RAMOFS || ils >= RAM_SIZE + RAMOFS - 72)
        return;

    uint8_t* tptr = ch32v003ram + (ils - RAMOFS);
    int w = 12, h = 6;
    int x, y;
    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            int py = window_h - y * 10 - 10;
            int px = window_w / 2 - 5 * 10 - 5 + x * 10 + ((x >= w / 2) ? 10 : -10);

            uint16_t tc = Coordmap[y + x * 8];
            int bit     = 1 << (tc >> 8);
            int row     = tc & 0xff;

            uint8_t* pptr = tptr + row;
            int intensity = 0;
            int i;
            for (i = 0; i < 8; i++)
            {
                if (bit & *pptr)
                    intensity += 1 << i;
                pptr += 9;
            }

            // Apply any color tuning.  Right now we're just stark white.
            CNFGColor(0x000000ff | (intensity << 24) | (intensity << 8) | (intensity << 16));
            CNFGTackRectangle(px - 4, py - 4, px + 4, py + 4);
        }
    }
}

int ch32v003WriteBitmapAsset(int slot, int asset_idx)
{
    size_t sz          = 0;
    const uint8_t* buf = cnfsGetFile(asset_idx, &sz);
    if (sz < 4)
    {
        printf("Error: Asset wrong size (%d) bytes.\n", (int)sz);
        return -1;
    }
    if (((const uint16_t*)buf)[0] != 12 || ((const uint16_t*)buf)[1] != 6)
    {
        printf("Error: Asset wrong dimensions (%d x %d) needs (12 x 6).\n", ((const uint16_t*)buf)[0],
               ((const uint16_t*)buf)[1]);
        return -1;
    }

    struct PixelMap
    {
        uint8_t buffer[6][12];
    };
    const struct PixelMap* pm = (const struct PixelMap*)(buf + 4);

    return ch32v003WriteBitmap(slot, pm->buffer);
}

int ch32v003WriteBitmap(int slot, const uint8_t pixels[6][12])
{
    if (slot >= CH32V003_MAX_IMAGE_SLOTS)
    {
        printf("Error: requested slot too big.\n");
        return -1;
    }

    uint8_t rkbuffer[128];

    int i, x, y;

    for (y = 0; y < 6; y++)
    {
        for (x = 0; x < 12; x++)
        {
            int intensity = pixels[y][x];
            int coord     = Coordmap[x * 8 + y];

            int ox = coord & 0xff;
            int oy = coord >> 8;

            int ofs       = ox;
            uint8_t* ledo = &rkbuffer[ofs];
            int imask     = ~(1 << oy);
            int mask      = ~imask;

            for (i = 0; i < 8; i++)
            {
                if (intensity & (1 << i))
                    *ledo |= mask;
                else
                    *ledo &= imask;
                ledo += 9;
            }
        }
    }

    // Make sure processor is halted if we're using it in framebuffer mode.
    // ch32v003SetReg(DMCONTROL, 0x80000001); // Request halt
    // ch32v003SetReg(DMCONTROL, 0x80000001); // Really request halt.
    // ch32v003SetReg(DMCONTROL, 0x00000001); // Clear halt request.

    return ch32v003WriteMemory(rkbuffer, sizeof(rkbuffer), 0x20000200 + slot * 72);
}

int ch32v003SelectBitmap(int slot)
{
    if (slot >= CH32V003_MAX_IMAGE_SLOTS)
    {
        printf("Error: requested slot too big.\n");
        return -1;
    }

    uint32_t ledPointer = 0x20000200 + slot * 72;
    // Overwrite DMA1_Channel5->MADDR, Assume we've been configured.
    return ch32v003WriteMemory((uint8_t*)&ledPointer, 4, 0x40020064);
}
