#ifndef _REG_H
#define _REG_H

#define REG_NUM 32

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
                      //r29,            $sp,      stac pointer
                      //r30             $fp,      frame pointer
                      //r31,            $ra,      return address
    REG_ACC,          //hi,lo                     accumulator 64-bit
    REG_FLOAT         //$f0-$f31                  floating arithmetic registers
} reg_type_t;

typedef struct reg_t {
    reg_type_t is;
    int r;
    char *name;
    char *alias;
} reg_t;

extern reg_t *arch[REG_NUM];
extern reg_t *arch_fp[REG_NUM];

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
reg_t *get_available_reg(reg_type_t type);
void release_reg();

#endif
