#include <stdio.h>

#include "reg.h"

reg_t *arch[REG_NUM];
reg_t *reg_pool[REG_NUM];

reg_t R_zero  = {.is = REG_ZERO,       .r = 0,  .name = "$zero", .alias = "r0" };
reg_t R_at  = {.is = REG_RESERVED_ASM, .r = 1,  .name = "$at",   .alias = "r1" };
reg_t R_v0  = {.is = REG_RESULT,       .r = 2,  .name = "$v0",   .alias = "r2" };
reg_t R_v1  = {.is = REG_RESULT,       .r = 3,  .name = "$v1",   .alias = "r3" };
reg_t R_a0  = {.is = REG_ARGUMENT,     .r = 4,  .name = "$a0",   .alias = "r4" };
reg_t R_a1  = {.is = REG_ARGUMENT,     .r = 5,  .name = "$a1",   .alias = "r5" };
reg_t R_a2  = {.is = REG_ARGUMENT,     .r = 6,  .name = "$a2",   .alias = "r6" };
reg_t R_a3  = {.is = REG_ARGUMENT,     .r = 7,  .name = "$a3",   .alias = "r7" };
reg_t R_t0  = {.is = REG_TEMP,         .r = 8,  .name = "$t0",   .alias = "r8" };
reg_t R_t1  = {.is = REG_TEMP,         .r = 9,  .name = "$t1",   .alias = "r9" };
reg_t R_t2 = {.is = REG_TEMP,         .r = 10, .name = "$t2",   .alias = "r10" };
reg_t R_t3 = {.is = REG_TEMP,         .r = 11, .name = "$t3",   .alias = "r11" };
reg_t R_t4 = {.is = REG_TEMP,         .r = 12, .name = "$t4",   .alias = "r12" };
reg_t R_t5 = {.is = REG_TEMP,         .r = 13, .name = "$t5",   .alias = "r13" };
reg_t R_t6 = {.is = REG_TEMP,         .r = 14, .name = "$t6",   .alias = "r14" };
reg_t R_t7 = {.is = REG_TEMP,         .r = 15, .name = "$t7",   .alias = "r15" };
reg_t R_s0 = {.is = REG_CONTENT,      .r = 16, .name = "$s0",   .alias = "r16" };
reg_t R_s1 = {.is = REG_CONTENT,      .r = 17, .name = "$s1",   .alias = "r17" };
reg_t R_s2 = {.is = REG_CONTENT,      .r = 18, .name = "$s2",   .alias = "r18" };
reg_t R_s3 = {.is = REG_CONTENT,      .r = 19, .name = "$s3",   .alias = "r19" };
reg_t R_s4 = {.is = REG_CONTENT,      .r = 20, .name = "$s4",   .alias = "r20" };
reg_t R_s5 = {.is = REG_CONTENT,      .r = 21, .name = "$s5",   .alias = "r21" };
reg_t R_s6 = {.is = REG_CONTENT,      .r = 22, .name = "$s6",   .alias = "r22" };
reg_t R_s7 = {.is = REG_CONTENT,      .r = 23, .name = "$s7",   .alias = "r23" };
reg_t R_t8 = {.is = REG_TEMP,         .r = 24, .name = "$t8",   .alias = "r24" };
reg_t R_t9 = {.is = REG_TEMP,         .r = 25, .name = "$t9",   .alias = "r25" };
reg_t R_k0 = {.is = REG_RESERVED_OS,  .r = 26, .name = "$k0",   .alias = "r26" };
reg_t R_k1 = {.is = REG_RESERVED_OS,  .r = 27, .name = "$k1",   .alias = "r27" };
reg_t R_gp = {.is = REG_POINTER,      .r = 28, .name = "$gp",   .alias = "r28" };
reg_t R_sp = {.is = REG_POINTER,      .r = 29, .name = "$sp",   .alias = "r29" };
reg_t R_fp = {.is = REG_POINTER,      .r = 30, .name = "$fp",   .alias = "r30" };
reg_t R_ra = {.is = REG_POINTER,      .r = 31, .name = "$ra",   .alias = "r31" };

reg_t R_hi = {.is = REG_POINTER,      .r = 32, .name = "$hi",   .alias = "r32" };
reg_t R_lo = {.is = REG_POINTER,      .r = 33, .name = "$lo",   .alias = "r33" };

void init_reg() {
    int i;

    arch[0]  = &R_zero;
    arch[1]  = &R_at;
    arch[2]  = &R_v0;
    arch[3]  = &R_v1;
    arch[4]  = &R_a0;
    arch[5]  = &R_a1;
    arch[6]  = &R_a2;
    arch[7]  = &R_a3;
    arch[8]  = &R_t0;
    arch[9]  = &R_t1;
    arch[10] = &R_t2;
    arch[11] = &R_t3;
    arch[12] = &R_t4;
    arch[13] = &R_t5;
    arch[14] = &R_t6;
    arch[15] = &R_t7;
    arch[16] = &R_s0;
    arch[17] = &R_s1;
    arch[18] = &R_s2;
    arch[19] = &R_s3;
    arch[20] = &R_s4;
    arch[21] = &R_s5;
    arch[22] = &R_s6;
    arch[23] = &R_s7;
    arch[24] = &R_t8;
    arch[25] = &R_t9;
    arch[26] = &R_k0;
    arch[27] = &R_k1;
    arch[28] = &R_gp;
    arch[29] = &R_sp;
    arch[30] = &R_fp;
    arch[31] = &R_ra;

    for(i=0;i<8;i++) {
        reg_pool[i] = NULL;
    }

    //we use only the $t0-$t9 and $s0-$s7 registers
    for(i=8;i<26;i++) {
        release_reg(arch[i]);
    }

    for(i=26;i<REG_NUM;i++) {
        reg_pool[i] = NULL;
    }

}

reg_t *get_available_reg(reg_type_t type) {
    int i;

    if (type==REG_CONTENT) {
        i = 16; //$s0

        while(!reg_pool[i]) {i++;}

        if (i<=23) { return reg_pool[i]; }

        return NULL;
    } else if (type==REG_TEMP) {
        i = 8; //$t0

        while(!reg_pool[i]) {i++;}

        if (i<=15) { return reg_pool[i]; }
        else if (reg_pool[24]) {return reg_pool[24];}

        return reg_pool[25];
    }

    return NULL;
}

void release_reg(reg_t *reg) {
    reg_pool[reg->r] = reg;
}

