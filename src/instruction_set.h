#ifndef _INSTRUCTION_SET
#define _INSTRUCTION_SET

struct instr_t;

#include "reg.h"

enum instr_type_t {
    INSTR_ISA,
    INSTR_PSEUDO_ISA
};

enum instr_format {
    FMT_RD_RS_RT,
    FMT_RD_RS,
    FMT_RS_RT,
    FMT_RD_RS_IMM_IMM,  // 5-bit each
    FMT_RD,
    FMT_RD_IMM,         //16-bit imm
    FMT_RD_IMM32,       //32-bit imm
    FMT_RD_LABEL,       //32-bit address
    FMT_RS,
    FMT_RD_RS_SHIFT,    // 5-bit imm
    FMT_RD_RS_IMM,      //16-bit imm
    FMT_LOAD,
    FMT_STORE,
    FMT_RS_RT_OFFSET,   //18-bit offset  (branch)
    FMT_RS_OFFSET,      //18-bit offset  (branch)
    FMT_ADDR,           //28-bit address (jump)
    FMT_OFFSET,         //18-bit offset  (branch)
    FMT_EMPTY
};

enum instr_data_t {
    I_FLOAT,
    I_INT,
    I_FLOAT_INT,
    I_INT_FLOAT
};

typedef struct mips_instr_t {
    enum instr_type_t type;
    enum instr_format fmt;
    enum instr_data_t datatype;
    char *name;
} mips_instr_t;

typedef struct instr_t {
    mips_instr_t *mips_instr;
    unsigned long id;

    char *label;
    struct reg_t *Rd;
    struct reg_t *Rs;
    struct reg_t *Rt;

    int ival; //possible offset, address, immediate
    char *goto_label;

    struct instr_t *prev;
    struct instr_t *next;
    struct instr_t *last;
} instr_t;

/* Instruction set */

//arithmetic
extern mips_instr_t I_add;
extern mips_instr_t I_addi;
extern mips_instr_t I_addiu;
extern mips_instr_t I_addu;
extern mips_instr_t I_lui;
extern mips_instr_t I_sub;
extern mips_instr_t I_subu;
extern mips_instr_t I_abs;

extern mips_instr_t I_la;
extern mips_instr_t I_move;
extern mips_instr_t I_clear;
extern mips_instr_t I_li;
extern mips_instr_t I_not;
extern mips_instr_t I_neg;
extern mips_instr_t I_negu;
extern mips_instr_t I_nop;

extern mips_instr_t I_seb;
extern mips_instr_t I_seh;
extern mips_instr_t I_clo;
extern mips_instr_t I_clz;

//shift
extern mips_instr_t I_sll;
extern mips_instr_t I_sllv;
extern mips_instr_t I_sra;
extern mips_instr_t I_srav;
extern mips_instr_t I_srl;
extern mips_instr_t I_srlv;

//rotate
extern mips_instr_t I_rotr;
extern mips_instr_t I_rotrv;

//logical
extern mips_instr_t I_and;
extern mips_instr_t I_andi;
extern mips_instr_t I_nor;
extern mips_instr_t I_or;
extern mips_instr_t I_ori;
extern mips_instr_t I_xor;
extern mips_instr_t I_xori;

extern mips_instr_t I_ext;
extern mips_instr_t I_ins;
extern mips_instr_t I_wsbh;

//condition test and set (NOT atomic)
extern mips_instr_t I_slt;
extern mips_instr_t I_slti;
extern mips_instr_t I_sltiu;
extern mips_instr_t I_sltu;
extern mips_instr_t I_seq;
extern mips_instr_t I_sne;
extern mips_instr_t I_sge;
extern mips_instr_t I_sgeu;
extern mips_instr_t I_sgt;
extern mips_instr_t I_sgtu;
extern mips_instr_t I_sle;
extern mips_instr_t I_sleu;

//condition test and move
extern mips_instr_t I_movn;
extern mips_instr_t I_movz;

//muldiv
extern mips_instr_t I_div;
extern mips_instr_t I_divu;
extern mips_instr_t I_mult;
extern mips_instr_t I_multu;
extern mips_instr_t I_rem;
extern mips_instr_t I_remu;

extern mips_instr_t I_madd;
extern mips_instr_t I_maddu;
extern mips_instr_t I_msub;
extern mips_instr_t I_msubu;

//acc
extern mips_instr_t I_mfhi;
extern mips_instr_t I_mflo;
extern mips_instr_t I_mthi;
extern mips_instr_t I_mtlo;

//branch
extern mips_instr_t I_beq;
extern mips_instr_t I_bne;

extern mips_instr_t I_bgez;
extern mips_instr_t I_bgtz;
extern mips_instr_t I_blez;
extern mips_instr_t I_bltz;

extern mips_instr_t I_bgezal;
extern mips_instr_t I_bltzal;

extern mips_instr_t I_b;
extern mips_instr_t I_bal;
extern mips_instr_t I_beqz;
extern mips_instr_t I_bnez;
extern mips_instr_t I_bgt;
extern mips_instr_t I_blt;
extern mips_instr_t I_bge;
extern mips_instr_t I_ble;

//jumps
extern mips_instr_t I_j;
extern mips_instr_t I_jal;
extern mips_instr_t I_jalr;
extern mips_instr_t I_jr;

//load
extern mips_instr_t I_lb;
extern mips_instr_t I_lbu;
extern mips_instr_t I_lh;
extern mips_instr_t I_lhu;
extern mips_instr_t I_lw;
extern mips_instr_t I_lwc1;
extern mips_instr_t I_lwl;
extern mips_instr_t I_lwr;
extern mips_instr_t I_l_s;
extern mips_instr_t I_ulw;

//store
extern mips_instr_t I_sb;
extern mips_instr_t I_sh;
extern mips_instr_t I_sw;
extern mips_instr_t I_swc1;
extern mips_instr_t I_swl;
extern mips_instr_t I_swr;
extern mips_instr_t I_s_s;
extern mips_instr_t I_usw;

//atomic
extern mips_instr_t I_ll;
extern mips_instr_t I_sc;

//syscall
extern mips_instr_t I_syscall;
extern mips_instr_t I_break;

//float single arithmetic
extern mips_instr_t I_add_s;
extern mips_instr_t I_sub_s;
extern mips_instr_t I_mul_s;
extern mips_instr_t I_div_s;
extern mips_instr_t I_abs_s;
extern mips_instr_t I_neg_s;
extern mips_instr_t I_mov_s;

extern mips_instr_t I_c_eq_s;
extern mips_instr_t I_c_le_s;
extern mips_instr_t I_c_lt_s;

//convert
extern mips_instr_t I_cvt_w_s;
extern mips_instr_t I_cvt_s_w;

//other c1
extern mips_instr_t I_bc1t;
extern mips_instr_t I_bc1f;

extern mips_instr_t I_mtc1;
extern mips_instr_t I_mfc1;

extern mips_instr_t I_ctc1;
extern mips_instr_t I_cfc1;

#endif
