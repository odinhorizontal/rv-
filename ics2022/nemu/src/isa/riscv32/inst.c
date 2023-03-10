/***************************************************************************************
 * Copyright (c) 2014-2022 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include "../../../monitor/ftrace.h"

enum
{
	TYPE_R,
	TYPE_I,
	TYPE_S,
	TYPE_B,
	TYPE_U,
	TYPE_J,
	TYPE_N, // none
};

#define src1R()           \
	do                    \
	{                     \
		*src1 = gpr(rs1); \
	} while (0)
#define src2R()           \
	do                    \
	{                     \
		*src2 = gpr(rs2); \
	} while (0)
#define immI()                            \
	do                                    \
	{                                     \
		*imm = SEXT(BITS(i, 31, 20), 12); \
	} while (0)
#define immU()                                  \
	do                                          \
	{                                           \
		*imm = SEXT(BITS(i, 31, 12), 20) << 12; \
	} while (0)
#define immS()                                                   \
	do                                                           \
	{                                                            \
		*imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); \
	} while (0)

#define immB()                                                            \
	do                                                                    \
	{                                                                     \
		*imm = (SEXT(BITS(i, 31, 31), 1) << 12) | (BITS(i, 7, 7) << 11) | \
			   (BITS(i, 30, 25) << 5) | (BITS(i, 11, 8) << 1);        \
	} while (0)
#define immJ()                                                              \
	do                                                                      \
	{                                                                       \
		*imm = (SEXT(BITS(i, 31, 31), 1) << 20) | (BITS(i, 19, 12) << 12) | \
			   (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1) | 0;        \
	} while (0)

void csrrwrs(word_t destination, word_t source1, word_t imm, bool tt)
{
	word_t t, *ptr = &gpr(0);
	if ( imm == 773 ) {
		ptr = &cpu.mtvec;
	} else if ( imm == 768 ) {
		ptr = &cpu.mstatus;
	} else if ( imm == 833 ) {
		ptr = &cpu.mepc;
	} else if ( imm == 834 ) {
		ptr = &cpu.mcause;
	}

	t = *ptr;
	if ( tt ) {
		*ptr = source1;
	} else {
		*ptr = t | source1;
	}
	gpr(destination) = t;
}

static void decode_operand(Decode *s, int *dest, word_t *src1, word_t *src2, word_t *imm, int type)
{
	uint32_t i = s->isa.inst.val;
	int rd = BITS(i, 11, 7);
	int rs1 = BITS(i, 19, 15);
	int rs2 = BITS(i, 24, 20);
	*dest = rd;
	switch (type)
	{
	case TYPE_R:
		src1R();
		src2R();
		break;
	case TYPE_I:
		src1R();
		immI();
		break;
	case TYPE_S:
		src1R();
		src2R();
		immS();
		break;
	case TYPE_B:
		src1R();
		src2R();
		immB();
		break;
	case TYPE_U:
		immU();
		break;
	case TYPE_J:
		immJ();
		break;
	}
}

void put_stack(Decode *s){
//todo: later

}
static int decode_exec(Decode *s)
{
	int destination = 0;
	word_t source1 = 0, source2 = 0, immediate = 0;
	s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */)                                  \
	{                                                                                         \
		decode_operand(s, &destination, &source1, &source2, &immediate, concat(TYPE_, type)); \
		__VA_ARGS__;                                                                          \
	}

	INSTPAT_START();
	INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui, U, gpr(destination) = immediate);
	INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw, I, gpr(destination) = vaddr_read(source1 + immediate, 4));
	INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw, S, vaddr_write(source1 + immediate, 4, source2));
	// INSTPAT("??????? ????? ????? 011 ????? 00000 11", ld, I, gpr(dest) = vaddr_read(src1 + src2, 8));
	INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi, I, gpr(destination) = source1 + immediate);
	INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc, U, gpr(destination) = s->pc + immediate);
	INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, gpr(destination) = s->pc + 4; s->dnpc = s->pc + immediate);

	// INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal, J, gpr(destination) = s->pc + 4; s->dnpc = s->pc + immediate); stack_call(s->pc, s->dnpc);
	INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, gpr(destination) = s->pc + 4; s->dnpc = (source1 + immediate) ; put_stack(s) );
	// INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, gpr(destination) = s->pc + 4; s->dnpc = (source1 + immediate) );
	// INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr, I, gpr(destination) = s->pc + 4; s->dnpc = (source1 + immediate) & (~1));

	INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu, B, if (source1 >= source2) s->dnpc = s->pc + immediate);
	INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge, B, if (((int)source1) >= ((int)source2)) s->dnpc = s->pc + immediate);
	INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt, B, if (((int)source1) < ((int)source2)) s->dnpc = s->pc + immediate);
	INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu, B, if ((source1) < (source2)) s->dnpc = s->pc + immediate);
	INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq, B, if (source1 == source2) s->dnpc = s->pc + immediate);
	INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne, B, if (source1 != source2) s->dnpc = s->pc + immediate);
	INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli, I, gpr(destination) = source1 << (immediate));
	INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu, I, gpr(destination) = (source1 < (word_t)immediate ? 1 : 0));
	INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti, I, gpr(destination) = ((int)source1 < (int)immediate ? 1 : 0));
	INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi, I, gpr(destination) = source1 & immediate);
	INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu, I, gpr(destination) = vaddr_read(source1 + immediate, 1));
	INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu, I, gpr(destination) = vaddr_read(source1 + immediate, 2));
	INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh, I, gpr(destination) = SEXT(vaddr_read(source1 + immediate, 2), 16));
	INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb, I, gpr(destination) = SEXT(vaddr_read(source1 + immediate, 1), 8));
	INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori, I, gpr(destination) = source1 ^ immediate);
	INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori, I, gpr(destination) = source1 | immediate);
	INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai, I, gpr(destination) = (int)source1 >> ((int)immediate));
	INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli, I, gpr(destination) = source1 >> (immediate));
	INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw, I, csrrwrs(destination, source1, immediate, true));
	INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs, I, csrrwrs(destination, source1, immediate, false));
	INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall, I, s->dnpc = isa_raise_intr(cpu.gpr[17], s->snpc));
	INSTPAT("0011000 00010 00000 000 00000 11100 11", mret, N, s->dnpc = cpu.mepc );
	INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add, R, gpr(destination) = source1 + source2);
	INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub, R, gpr(destination) = source1 - source2);
	INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul, R, gpr(destination) = source1 * source2);
	INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu, R, gpr(destination) = (((long long)source1 * (long long)source2) >> 32));
	INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh, R, gpr(destination) = (int)((SEXT((long long)source1, 32) * SEXT((long long)source2, 32)) >> 32));
	INSTPAT("0000001 ????? ????? 010 ????? 01100 11", mulhsu, R, gpr(destination) = (((long long)source1 * SEXT((long long)source2, 32)) >> 32));
	INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div, R, gpr(destination) = (int)source1 / (int)source2);
	INSTPAT("0000001 ????? ????? 101 ????? 01100 11", divu, R, gpr(destination) = source1 / source2);
	INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem, R, gpr(destination) = (int)source1 % (int)source2);
	INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu, R, gpr(destination) = source1 % source2);
	INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll, R, gpr(destination) = source1 << source2);
	INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra, R, gpr(destination) = (int)source1 >> ((int)source2));
	INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl, R, gpr(destination) = source1 >> (source2));
	INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and, R, gpr(destination) = source1 & source2);
	INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu, R, gpr(destination) = (source1 < source2 ? 1 : 0));
	INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt, R, gpr(destination) = ((int)source1 < (int)source2 ? 1 : 0));
	INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or, R, gpr(destination) = source1 | source2);
	INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor, R, gpr(destination) = source1 ^ source2);
	INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb, S, vaddr_write(source1 + immediate, 1, source2));
	INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh, S, vaddr_write(source1 + immediate, 2, source2));
	INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, gpr(10))); // R(10) is $a0
	INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv, N, INV(s->pc));
	INSTPAT_END();

	gpr(0) = 0; // reset $zero to 0

	return 0;
}

int isa_exec_once(Decode *s)
{
	s->isa.inst.val = inst_fetch(&s->snpc, 4);
	if (s->pc == 0x830000e8)
		decode_exec(s);
	else
		decode_exec(s);
	return 0;
}
