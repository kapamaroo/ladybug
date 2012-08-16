#include "instruction_set.h"

#include "build_flags.h"

//arithmetic
mips_instr_t I_add   = {.name="add",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_addi  = {.name="addi",  .fmt=FMT_RD_RS_IMM,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_addiu = {.name="addiu", .fmt=FMT_RD_RS_IMM,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_addu  = {.name="addu",  .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_lui   = {.name="lui",   .fmt=FMT_RD_IMM,        .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_sub   = {.name="sub",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_subu  = {.name="subu",  .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_abs   = {.name="abs",   .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_ISA};

//expand to lui, ori

#if (USE_PSEUDO_INSTR_LA==1)
mips_instr_t I_la    = {.name="la",    .fmt=FMT_RD_LABEL,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //load address
#endif

mips_instr_t I_li    = {.name="li",    .fmt=FMT_RD_IMM32,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //load 32bit imm

mips_instr_t I_move  = {.name="move",  .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //convert to add
mips_instr_t I_clear = {.name="clear", .fmt=FMT_RD,            .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //convert to add
mips_instr_t I_not   = {.name="not",   .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //convert to nor
mips_instr_t I_neg   = {.name="neg",   .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //convert to sub
mips_instr_t I_negu  = {.name="negu",  .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //convert to subu
mips_instr_t I_nop   = {.name="nop",   .fmt=FMT_EMPTY,         .datatype=I_INT, .type=INSTR_PSEUDO_ISA};

mips_instr_t I_seb = {.name="seb",     .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_ISA};  //sign extend byte in register
mips_instr_t I_seh = {.name="seh",     .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_ISA};  //sign extend halfword in register
mips_instr_t I_clo = {.name="clo",     .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_ISA};          //count leading ones
mips_instr_t I_clz = {.name="clz",     .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_ISA};          //count leading zeroes

//shift
mips_instr_t I_sll  = {.name="sll",    .fmt=FMT_RD_RS_SHIFT,   .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_sllv = {.name="sllv",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_sra  = {.name="sra",    .fmt=FMT_RD_RS_SHIFT,   .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_srav = {.name="srav",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_srl  = {.name="srl",    .fmt=FMT_RD_RS_SHIFT,   .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_srlv = {.name="srlv",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};

//rotate
mips_instr_t I_rotr  = {.name="rotr",  .fmt=FMT_RD_RS_SHIFT,   .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_rotrv = {.name="rotrv", .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};

//logical
mips_instr_t I_and  = {.name="and",    .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_andi = {.name="andi",   .fmt=FMT_RD_RS_IMM,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_nor  = {.name="nor",    .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_or   = {.name="or",     .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_ori  = {.name="ori",    .fmt=FMT_RD_RS_IMM,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_xor  = {.name="xor",    .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_xori = {.name="xori",   .fmt=FMT_RD_RS_IMM,     .datatype=I_INT, .type=INSTR_ISA};

mips_instr_t I_ext  = {.name="ext",    .fmt=FMT_RD_RS_IMM_IMM, .datatype=I_INT, .type=INSTR_ISA};        //extract bits
mips_instr_t I_ins  = {.name="ins",    .fmt=FMT_RD_RS_IMM_IMM, .datatype=I_INT, .type=INSTR_ISA};        //insert bits
mips_instr_t I_wsbh = {.name="wsbh",   .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_ISA};        //byte swap within halfwords

//condition test and set (NOT atomic)
mips_instr_t I_slt   = {.name="slt",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_slti  = {.name="slti",  .fmt=FMT_RD_RS_IMM,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_sltiu = {.name="sltiu", .fmt=FMT_RD_RS_IMM,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_sltu  = {.name="sltu",  .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_seq   = {.name="seq",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_sne   = {.name="sne",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_sge   = {.name="sge",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_sgeu  = {.name="sgeu",  .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_sgt   = {.name="sgt",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_sgtu  = {.name="sgtu",  .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_sle   = {.name="sle",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_sleu  = {.name="sleu",  .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};

//condition test and move
mips_instr_t I_movn = {.name="movn",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};          //mips 4
mips_instr_t I_movz = {.name="movz",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_ISA};          //mips 4

//muldiv
mips_instr_t I_div   = {.name="div",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_divu  = {.name="divu",  .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_mult  = {.name="mult",  .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_multu = {.name="multu", .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_rem   = {.name="rem",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //expand to use mfhi (remainder)
mips_instr_t I_remu  = {.name="rem",   .fmt=FMT_RD_RS_RT,      .datatype=I_INT, .type=INSTR_PSEUDO_ISA};   //expand to use mfhi (remainder)

mips_instr_t I_madd  = {.name="madd",  .fmt=FMT_RS_RT,         .datatype=I_INT, .type=INSTR_ISA};          //mips 4
mips_instr_t I_maddu = {.name="maddu", .fmt=FMT_RS_RT,         .datatype=I_INT, .type=INSTR_ISA};          //mips 4
mips_instr_t I_msub  = {.name="msub",  .fmt=FMT_RS_RT,         .datatype=I_INT, .type=INSTR_ISA};          //mips 4
mips_instr_t I_msubu = {.name="msubu", .fmt=FMT_RS_RT,         .datatype=I_INT, .type=INSTR_ISA};          //mips 4

//acc
mips_instr_t I_mfhi = {.name="mfhi",   .fmt=FMT_RD,            .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_mflo = {.name="mhlo",   .fmt=FMT_RD,            .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_mthi = {.name="mthi",   .fmt=FMT_RS,            .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_mtlo = {.name="mtlo",   .fmt=FMT_RS,            .datatype=I_INT, .type=INSTR_ISA};

//branch
mips_instr_t I_beq  = {.name="beq",    .fmt=FMT_RS_RT_OFFSET,  .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_bne  = {.name="bne",    .fmt=FMT_RS_RT_OFFSET,  .datatype=I_INT, .type=INSTR_ISA};

mips_instr_t I_bgez = {.name="bgez",   .fmt=FMT_RS_OFFSET,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_bgtz = {.name="bgtz",   .fmt=FMT_RS_OFFSET,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_blez = {.name="blez",   .fmt=FMT_RS_OFFSET,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_bltz = {.name="bltz",   .fmt=FMT_RS_OFFSET,     .datatype=I_INT, .type=INSTR_ISA};

mips_instr_t I_bgezal = {.name="bgezal", .fmt=FMT_RS_OFFSET,   .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_bltzal = {.name="bltzal", .fmt=FMT_RS_OFFSET,   .datatype=I_INT, .type=INSTR_ISA};

mips_instr_t I_bc1t = {.name="bc1t",     .fmt=FMT_OFFSET,      .datatype=I_INT, .type=INSTR_ISA};   //test (cc=0) control bit (cc==1)
mips_instr_t I_bc1f = {.name="bc1f",     .fmt=FMT_OFFSET,      .datatype=I_INT, .type=INSTR_ISA};   //test (cc=0) control bit (cc==0)

mips_instr_t I_b    = {.name="b",      .fmt=FMT_OFFSET,        .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_bal  = {.name="bal",    .fmt=FMT_OFFSET,        .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_beqz = {.name="beqz",   .fmt=FMT_RS_OFFSET,     .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_bnez = {.name="bnez",   .fmt=FMT_RS_OFFSET,     .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_bgt  = {.name="bgt",    .fmt=FMT_RS_RT_OFFSET,  .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_blt  = {.name="blt",    .fmt=FMT_RS_RT_OFFSET,  .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_bge  = {.name="bge",    .fmt=FMT_RS_RT_OFFSET,  .datatype=I_INT, .type=INSTR_PSEUDO_ISA};
mips_instr_t I_ble  = {.name="ble",    .fmt=FMT_RS_RT_OFFSET,  .datatype=I_INT, .type=INSTR_PSEUDO_ISA};

//jumps
mips_instr_t I_j    = {.name="j",      .fmt=FMT_ADDR,          .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_jal  = {.name="jal",    .fmt=FMT_ADDR,          .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_jalr = {.name="jalr",   .fmt=FMT_RD_RS,         .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_jr   = {.name="jr",     .fmt=FMT_RS,            .datatype=I_INT, .type=INSTR_ISA};

//load
mips_instr_t I_lb   = {.name="lb",    .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_lbu  = {.name="lbu",   .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_lh   = {.name="lh",    .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_lhu  = {.name="lhu",   .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_lw   = {.name="lw",    .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_lwl  = {.name="lwl",   .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_ISA};         //load (for unaligned data)
mips_instr_t I_lwr  = {.name="lwr",   .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_ISA};         //load (for unaligned data)
mips_instr_t I_ulw  = {.name="ulw",   .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_PSEUDO_ISA};  //load unaligned data

//store
mips_instr_t I_sb   = {.name="sb",    .fmt=FMT_STORE,          .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_sh   = {.name="sh",    .fmt=FMT_STORE,          .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_sw   = {.name="sw",    .fmt=FMT_STORE,          .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_swl  = {.name="swl",   .fmt=FMT_STORE,          .datatype=I_INT, .type=INSTR_ISA};         //store (for unaligned data)
mips_instr_t I_swr  = {.name="swr",   .fmt=FMT_STORE,          .datatype=I_INT, .type=INSTR_ISA};         //store (for unaligned data)
mips_instr_t I_usw  = {.name="usw",   .fmt=FMT_STORE,          .datatype=I_INT, .type=INSTR_PSEUDO_ISA};  //store unaligned data

//atomic
mips_instr_t I_ll = {.name="ll",      .fmt=FMT_LOAD,           .datatype=I_INT, .type=INSTR_ISA};         //load atomic (mips 2)
mips_instr_t I_sc = {.name="sc",      .fmt=FMT_STORE,          .datatype=I_INT, .type=INSTR_ISA};         //store atomic (mips 2)

//float load, store
mips_instr_t I_lwc1 = {.name="lwc1",  .fmt=FMT_LOAD,           .datatype=I_FLOAT_INT, .type=INSTR_ISA};         //FLOAT
mips_instr_t I_swc1 = {.name="swc1",  .fmt=FMT_STORE,          .datatype=I_FLOAT_INT, .type=INSTR_ISA};         //FLOAT

//float single arithmetic
mips_instr_t I_add_s = {.name="add.s",   .fmt=FMT_RD_RS_RT,    .datatype=I_FLOAT, .type=INSTR_ISA};
mips_instr_t I_sub_s = {.name="sub.s",   .fmt=FMT_RD_RS_RT,    .datatype=I_FLOAT, .type=INSTR_ISA};
mips_instr_t I_mul_s = {.name="mul.s",   .fmt=FMT_RD_RS_RT,    .datatype=I_FLOAT, .type=INSTR_ISA};
mips_instr_t I_div_s = {.name="div.s",   .fmt=FMT_RD_RS_RT,    .datatype=I_FLOAT, .type=INSTR_ISA};
mips_instr_t I_abs_s = {.name="abs.s",   .fmt=FMT_RD_RS,       .datatype=I_FLOAT, .type=INSTR_ISA};
mips_instr_t I_neg_s = {.name="neg.s",   .fmt=FMT_RD_RS,       .datatype=I_FLOAT, .type=INSTR_ISA};
mips_instr_t I_mov_s = {.name="mov.s",   .fmt=FMT_RD_RS,       .datatype=I_FLOAT, .type=INSTR_ISA};    //move fpr to fpr

mips_instr_t I_c_eq_s = {.name="c.eq.s", .fmt=FMT_RS_RT,       .datatype=I_FLOAT, .type=INSTR_ISA};    //eq with bc1t, neq wth bc1f
mips_instr_t I_c_le_s = {.name="c.le.s", .fmt=FMT_RS_RT,       .datatype=I_FLOAT, .type=INSTR_ISA};    //le with bc1t, gt wth bc1f
mips_instr_t I_c_lt_s = {.name="c.lt.s", .fmt=FMT_RS_RT,       .datatype=I_FLOAT, .type=INSTR_ISA};    //lt with bc1t, ge wth bc1f

//mips_instr_t I_c_nlt_s = {.name="c.ge.s",  .fmt=FMT_RS_RT,       .datatype=I_FLOAT, .type=INSTR_ISA};
//mips_instr_t I_c_nle_s = {.name="c.gt.s",  .fmt=FMT_RS_RT,       .datatype=I_FLOAT, .type=INSTR_ISA};
//mips_instr_t I_c_neq_s = {.name="c.neq.s", .fmt=FMT_RS_RT,       .datatype=I_FLOAT, .type=INSTR_ISA};

//other c1
mips_instr_t I_mtc1 = {.name="mtc1",     .fmt=FMT_RS_RT,       .datatype=I_INT_FLOAT, .type=INSTR_ISA};    //move gpr to fpr
mips_instr_t I_mfc1 = {.name="mfc1",     .fmt=FMT_RS_RT,       .datatype=I_INT_FLOAT, .type=INSTR_ISA};    //move fpr to gpr
mips_instr_t I_ctc1 = {.name="ctc1",     .fmt=FMT_RS_RT,       .datatype=I_INT_FLOAT, .type=INSTR_ISA};    //move gpr to fcr
mips_instr_t I_cfc1 = {.name="cfc1",     .fmt=FMT_RS_RT,       .datatype=I_INT_FLOAT, .type=INSTR_ISA};    //move fcr to gpr

//syscall
mips_instr_t I_syscall = {.name="syscall", .fmt=FMT_EMPTY,     .datatype=I_INT, .type=INSTR_ISA};
mips_instr_t I_break   = {.name="break",   .fmt=FMT_EMPTY,     .datatype=I_INT, .type=INSTR_ISA};

//convert
mips_instr_t I_cvt_w_s = {.name="cvt.w.s", .fmt=FMT_RD_RS,     .datatype=I_FLOAT, .type=INSTR_ISA};
mips_instr_t I_cvt_s_w = {.name="cvt.s.w", .fmt=FMT_RD_RS,     .datatype=I_FLOAT, .type=INSTR_ISA};
