#ifndef _REG_H
#define _REG_H

struct reg_t;

#include "instruction_set.h"

#define ARCH_REG_NUM 32
#define ARCH_FP_REG_NUM 32

#define ARCH_REG_CONTENT_NUM 8
#define ARCH_REG_TEMP_NUM 10

typedef enum reg_status_t {
    REG_VIRT,         //before register allocation
    REG_ALLOCATED,    //after  register allocation
    REG_PHYSICAL      //arch registers
} reg_status_t;

typedef enum reg_type_t {
    REG_ZERO,         //r0,             $zero     always 0
    REG_RESERVED_ASM, //r1,             $at       reserved for assembler
    REG_RESERVED_OS,  //r26-r27,        $k0-$k1,  reserved for OS
    REG_RESULT,       //r2-r3,          $v0-$v1,  stores results
    REG_ARGUMENT,     //r4-r7,          $a0-$a3,  stores arguments
    REG_TEMP,         //r8-r15,         $t0-$t7,  temps, not saved
                      //r24-r25         $t8-$t9,  more temps
    REG_CONTENT,      //r16-r23         $s0-$s7,  contents saved for use later
    REG_POINTER,      //r28,            $gp,      global pointer
                      //r29,            $sp,      stack pointer
                      //r30             $fp,      frame pointer
                      //r31,            $ra,      return address
    REG_ACC,          //hi,lo                     accumulator 64-bit
    REG_FLOAT         //$f0-$f31                  floating arithmetic registers
} reg_type_t;

typedef struct reg_t {
    reg_status_t type;
    reg_type_t is;

    unsigned long virtual;   //unique number for REG_VIRT

    int r;                  //predefined arch number for physical registers
    char *name;
    char *alias;

    struct instr_t *live;
    struct instr_t *die;
    struct reg_t *physical; //physical register
} reg_t;

#define IS_REG_VIRT(reg) (reg && reg->type == REG_VIRT)

extern reg_t *arch[ARCH_REG_NUM];
extern reg_t *arch_fp[ARCH_FP_REG_NUM];

extern reg_t R_zero;
extern reg_t R_at;
extern reg_t R_v0;
extern reg_t R_v1;
extern reg_t R_a0;
extern reg_t R_a1;
extern reg_t R_a2;
extern reg_t R_a3;
extern reg_t R_t0;
extern reg_t R_t1;
extern reg_t R_t2;
extern reg_t R_t3;
extern reg_t R_t4;
extern reg_t R_t5;
extern reg_t R_t6;
extern reg_t R_t7;
extern reg_t R_s0;
extern reg_t R_s1;
extern reg_t R_s2;
extern reg_t R_s3;
extern reg_t R_s4;
extern reg_t R_s5;
extern reg_t R_s6;
extern reg_t R_s7;
extern reg_t R_t8;
extern reg_t R_t9;
extern reg_t R_k0;
extern reg_t R_k1;
extern reg_t R_gp;
extern reg_t R_sp;
extern reg_t R_fp;
extern reg_t R_ra;

void init_reg();
void free_all_registers();
reg_t *get_available_reg(reg_type_t type);
void release_reg(reg_t *reg);
void reg_liveness_analysis(struct instr_t *pc);

#endif
