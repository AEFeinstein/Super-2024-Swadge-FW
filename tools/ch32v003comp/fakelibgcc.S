/* Integer division routines for RISC-V.

   Copyright (C) 2016-2025 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */


#define FUNC_TYPE(X)	.type X,@function
#define FUNC_SIZE(X)	.size X,.-X

#define FUNC_BEGIN(X)		\
	.align 2;		\
	.globl X;		\
	FUNC_TYPE (X);		\
X:				\
	LPAD

#define FUNC_END(X)		\
	FUNC_SIZE(X)

#define FUNC_ALIAS(X,Y)		\
	.globl X;		\
	X = Y

#define CONCAT1(a, b)		CONCAT2(a, b)
#define CONCAT2(a, b)		a ## b
#define HIDDEN_JUMPTARGET(X)	CONCAT1(__hidden_, X)
#define HIDDEN_DEF(X)		FUNC_ALIAS(HIDDEN_JUMPTARGET(X), X);     \
				.hidden HIDDEN_JUMPTARGET(X)

/* GNU_PROPERTY_RISCV64_* macros from elf.h for use in asm code.  */
#define FEATURE_1_AND 0xc0000000
#define FEATURE_1_FCFI 1
#define FEATURE_1_BCFI 2

/* Add a NT_GNU_PROPERTY_TYPE_0 note.  */
#if __riscv_xlen == 32
#  define GNU_PROPERTY(type, value)	\
    .section .note.gnu.property, "a";	\
    .p2align 2;				\
    .word 4;				\
    .word 12;				\
    .word 5;				\
    .asciz "GNU";			\
    .word type;				\
    .word 4;				\
    .word value;			\
    .text
#else
#  define GNU_PROPERTY(type, value)	\
    .section .note.gnu.property, "a";	\
    .p2align 3;				\
    .word 4;				\
    .word 16;				\
    .word 5;				\
    .asciz "GNU";			\
    .word type;				\
    .word 4;				\
    .word value;			\
    .word 0;				\
    .text
#endif

/* Add GNU property note with the supported features to all asm code
   where sysdep.h is included.  */
#undef __VALUE_FOR_FEATURE_1_AND
#if defined (__riscv_zicfilp) || defined (__riscv_zicfiss)
#  if defined (__riscv_zicfilp)
#    if defined (__riscv_zicfiss)
#      define __VALUE_FOR_FEATURE_1_AND 0x3
#    else
#      define __VALUE_FOR_FEATURE_1_AND 0x1
#    endif
#  else
#    if defined (__riscv_zicfiss)
#      define __VALUE_FOR_FEATURE_1_AND 0x2
#    else
#      error "What?"
#    endif
#  endif
#endif

#if defined (__VALUE_FOR_FEATURE_1_AND)
GNU_PROPERTY (FEATURE_1_AND, __VALUE_FOR_FEATURE_1_AND)
#endif
#undef __VALUE_FOR_FEATURE_1_AND

#ifdef __riscv_zicfilp
# define SET_LPAD   lui  t2, 0
# define LPAD       lpad 0
#else
# define SET_LPAD
# define LPAD
#endif

  .text
  .align 2

#if __riscv_xlen == 32
/* Our RV64 64-bit routines are equivalent to our RV32 32-bit routines.  */
# define __udivdi3 __udivsi3
# define __umoddi3 __umodsi3
# define __divdi3 __divsi3
# define __moddi3 __modsi3
#else
FUNC_BEGIN (__udivsi3)
  /* Compute __udivdi3(a0 << 32, a1 << 32); cast result to uint32_t.  */
  sll    a0, a0, 32
  sll    a1, a1, 32
  move   t0, ra
  jal    HIDDEN_JUMPTARGET(__udivdi3)
  sext.w a0, a0
  jr     t0
FUNC_END (__udivsi3)

FUNC_BEGIN (__umodsi3)
  /* Compute __udivdi3((uint32_t)a0, (uint32_t)a1); cast a1 to uint32_t.  */
  sll    a0, a0, 32
  sll    a1, a1, 32
  srl    a0, a0, 32
  srl    a1, a1, 32
  move   t0, ra
  jal    HIDDEN_JUMPTARGET(__udivdi3)
  sext.w a0, a1
  jr     t0
FUNC_END (__umodsi3)

FUNC_ALIAS (__modsi3, __moddi3)

FUNC_BEGIN( __divsi3)
  /* Check for special case of INT_MIN/-1. Otherwise, fall into __divdi3.  */
  li    t0, -1
  beq   a1, t0, .L20
#endif

FUNC_BEGIN (__divdi3)
  bltz  a0, .L10
  bltz  a1, .L11
  /* Since the quotient is positive, fall into __udivdi3.  */

FUNC_BEGIN (__udivdi3)
  mv    a2, a1
  mv    a1, a0
  li    a0, -1
  beqz  a2, .L5
  li    a3, 1
  bgeu  a2, a1, .L2
.L1:
  blez  a2, .L2
  slli  a2, a2, 1
  slli  a3, a3, 1
  bgtu  a1, a2, .L1
.L2:
  li    a0, 0
.L3:
  bltu  a1, a2, .L4
  sub   a1, a1, a2
  or    a0, a0, a3
.L4:
  srli  a3, a3, 1
  srli  a2, a2, 1
  bnez  a3, .L3
.L5:
  ret
FUNC_END (__udivdi3)
HIDDEN_DEF (__udivdi3)

FUNC_BEGIN (__umoddi3)
  /* Call __udivdi3(a0, a1), then return the remainder, which is in a1.  */
  move  t0, ra
  jal   HIDDEN_JUMPTARGET(__udivdi3)
  move  a0, a1
  jr    t0
FUNC_END (__umoddi3)

  /* Handle negative arguments to __divdi3.  */
.L10:
  neg   a0, a0
  /* Zero is handled as a negative so that the result will not be inverted.  */
  bgtz  a1, .L12     /* Compute __udivdi3(-a0, a1), then negate the result.  */

  neg   a1, a1
  j     HIDDEN_JUMPTARGET(__udivdi3)     /* Compute __udivdi3(-a0, -a1).  */
.L11:                /* Compute __udivdi3(a0, -a1), then negate the result.  */
  neg   a1, a1
.L12:
  move  t0, ra
  jal   HIDDEN_JUMPTARGET(__udivdi3)
  neg   a0, a0
  jr    t0
FUNC_END (__divdi3)

FUNC_BEGIN (__moddi3)
  move   t0, ra
  bltz   a1, .L31
  bltz   a0, .L32
.L30:
  jal    HIDDEN_JUMPTARGET(__udivdi3)    /* The dividend is not negative.  */
  move   a0, a1
  jr     t0
.L31:
  neg    a1, a1
  bgez   a0, .L30
.L32:
  neg    a0, a0
  jal    HIDDEN_JUMPTARGET(__udivdi3)    /* The dividend is hella negative.  */
  neg    a0, a1
  jr     t0
FUNC_END (__moddi3)

#if __riscv_xlen == 64
  /* continuation of __divsi3 */
.L20:
  sll   t0, t0, 31
  bne   a0, t0, __divdi3
  ret
FUNC_END (__divsi3)
#endif





FUNC_BEGIN (__mulsi3)
        mv      a5,a0
        li      a0,0
.L2ms:
        bne     a5,zero,.L3ms
        ret
.L3ms:
        slli    a3,a5,31
        srai    a4,a3,31
        and     a4,a4,a1
        add     a0,a0,a4
        srli    a5,a5,1
        slli    a1,a1,1
        j       .L2ms
FUNC_END (__mulsi3)


FUNC_BEGIN (__clzsi2)
        li      a3,5
        li      a4,16
        li      a5,32
        li      a1,1
.L5cs:
        sra     a2,a0,a4
        beq     a2,zero,.L2cs
        bne     a4,a1,.L3cs
        addi    a0,a5,-2
        ret
.L3cs:
        sub     a5,a5,a4
        mv      a0,a2
.L2cs:
        addi    a3,a3,-1
        srai    a4,a4,1
        bne     a3,zero,.L5cs
        sub     a0,a5,a0
        ret
FUNC_END (__clzsi2)
