#include <stdio.h>

#include "reg.h"

reg_t *arch[ARCH_REG_NUM];
reg_t *arch_fp[ARCH_FP_REG_NUM];

reg_t *reg_pool_content[ARCH_REG_CONTENT_NUM];   //$s0-$s7
reg_t *reg_pool_temp[ARCH_REG_TEMP_NUM];         //$t0-$t7,$t8-$t9
reg_t *reg_pool_float[ARCH_FP_REG_NUM];          //$f0-$f31

reg_t R_zero  = {.type = REG_PHYSICAL, .is = REG_ZERO,       .r = 0,  .name = "$zero", .alias = "r0" };
reg_t R_at  = {.type = REG_PHYSICAL, .is = REG_RESERVED_ASM, .r = 1,  .name = "$at",   .alias = "r1" };
reg_t R_v0  = {.type = REG_PHYSICAL, .is = REG_RESULT,       .r = 2,  .name = "$v0",   .alias = "r2" };
reg_t R_v1  = {.type = REG_PHYSICAL, .is = REG_RESULT,       .r = 3,  .name = "$v1",   .alias = "r3" };
reg_t R_a0  = {.type = REG_PHYSICAL, .is = REG_ARGUMENT,     .r = 4,  .name = "$a0",   .alias = "r4" };
reg_t R_a1  = {.type = REG_PHYSICAL, .is = REG_ARGUMENT,     .r = 5,  .name = "$a1",   .alias = "r5" };
reg_t R_a2  = {.type = REG_PHYSICAL, .is = REG_ARGUMENT,     .r = 6,  .name = "$a2",   .alias = "r6" };
reg_t R_a3  = {.type = REG_PHYSICAL, .is = REG_ARGUMENT,     .r = 7,  .name = "$a3",   .alias = "r7" };
reg_t R_t0  = {.type = REG_PHYSICAL, .is = REG_TEMP,         .r = 0,  .name = "$t0",   .alias = "r8" };
reg_t R_t1  = {.type = REG_PHYSICAL, .is = REG_TEMP,         .r = 1,  .name = "$t1",   .alias = "r9" };
reg_t R_t2 = {.type = REG_PHYSICAL, .is = REG_TEMP,          .r = 2, .name = "$t2",   .alias = "r10" };
reg_t R_t3 = {.type = REG_PHYSICAL, .is = REG_TEMP,          .r = 3, .name = "$t3",   .alias = "r11" };
reg_t R_t4 = {.type = REG_PHYSICAL, .is = REG_TEMP,          .r = 4, .name = "$t4",   .alias = "r12" };
reg_t R_t5 = {.type = REG_PHYSICAL, .is = REG_TEMP,          .r = 5, .name = "$t5",   .alias = "r13" };
reg_t R_t6 = {.type = REG_PHYSICAL, .is = REG_TEMP,          .r = 6, .name = "$t6",   .alias = "r14" };
reg_t R_t7 = {.type = REG_PHYSICAL, .is = REG_TEMP,          .r = 7, .name = "$t7",   .alias = "r15" };
reg_t R_s0 = {.type = REG_PHYSICAL, .is = REG_CONTENT,       .r = 0, .name = "$s0",   .alias = "r16" };
reg_t R_s1 = {.type = REG_PHYSICAL, .is = REG_CONTENT,       .r = 1, .name = "$s1",   .alias = "r17" };
reg_t R_s2 = {.type = REG_PHYSICAL, .is = REG_CONTENT,       .r = 2, .name = "$s2",   .alias = "r18" };
reg_t R_s3 = {.type = REG_PHYSICAL, .is = REG_CONTENT,       .r = 3, .name = "$s3",   .alias = "r19" };
reg_t R_s4 = {.type = REG_PHYSICAL, .is = REG_CONTENT,       .r = 4, .name = "$s4",   .alias = "r20" };
reg_t R_s5 = {.type = REG_PHYSICAL, .is = REG_CONTENT,       .r = 5, .name = "$s5",   .alias = "r21" };
reg_t R_s6 = {.type = REG_PHYSICAL, .is = REG_CONTENT,       .r = 6, .name = "$s6",   .alias = "r22" };
reg_t R_s7 = {.type = REG_PHYSICAL, .is = REG_CONTENT,       .r = 7, .name = "$s7",   .alias = "r23" };
reg_t R_t8 = {.type = REG_PHYSICAL, .is = REG_TEMP,          .r = 8, .name = "$t8",   .alias = "r24" };
reg_t R_t9 = {.type = REG_PHYSICAL, .is = REG_TEMP,          .r = 9, .name = "$t9",   .alias = "r25" };
reg_t R_k0 = {.type = REG_PHYSICAL, .is = REG_RESERVED_OS,   .r = 26, .name = "$k0",   .alias = "r26" };
reg_t R_k1 = {.type = REG_PHYSICAL, .is = REG_RESERVED_OS,   .r = 27, .name = "$k1",   .alias = "r27" };
reg_t R_gp = {.type = REG_PHYSICAL, .is = REG_POINTER,       .r = 28, .name = "$gp",   .alias = "r28" };
reg_t R_sp = {.type = REG_PHYSICAL, .is = REG_POINTER,       .r = 29, .name = "$sp",   .alias = "r29" };
reg_t R_fp = {.type = REG_PHYSICAL, .is = REG_POINTER,       .r = 30, .name = "$fp",   .alias = "r30" };
reg_t R_ra = {.type = REG_PHYSICAL, .is = REG_POINTER,       .r = 31, .name = "$ra",   .alias = "r31" };

reg_t R_lo = {.type = REG_PHYSICAL, .is = REG_ACC,           .r = 0, .name = "$lo",   .alias = "acc0" };
reg_t R_hi = {.type = REG_PHYSICAL, .is = REG_ACC,           .r = 1, .name = "$hi",   .alias = "acc1" };

reg_t FP_0 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 0, .name = "$f0",   .alias = "fp0" };
reg_t FP_1 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 1, .name = "$f1",   .alias = "fp1" };
reg_t FP_2 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 2, .name = "$f2",   .alias = "fp2" };
reg_t FP_3 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 3, .name = "$f3",   .alias = "fp3" };
reg_t FP_4 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 4, .name = "$f4",   .alias = "fp4" };
reg_t FP_5 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 5, .name = "$f5",   .alias = "fp5" };
reg_t FP_6 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 6, .name = "$f6",   .alias = "fp6" };
reg_t FP_7 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 7, .name = "$f7",   .alias = "fp7" };
reg_t FP_8 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 8, .name = "$f8",   .alias = "fp8" };
reg_t FP_9 = {.type = REG_PHYSICAL, .is = REG_FLOAT,         .r = 9, .name = "$f9",   .alias = "fp9" };
reg_t FP_10 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 10, .name = "$f10",   .alias = "fp10" };
reg_t FP_11 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 11, .name = "$f11",   .alias = "fp11" };
reg_t FP_12 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 12, .name = "$f12",   .alias = "fp12" };
reg_t FP_13 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 13, .name = "$f13",   .alias = "fp13" };
reg_t FP_14 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 14, .name = "$f14",   .alias = "fp14" };
reg_t FP_15 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 15, .name = "$f15",   .alias = "fp15" };
reg_t FP_16 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 16, .name = "$f16",   .alias = "fp16" };
reg_t FP_17 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 17, .name = "$f17",   .alias = "fp17" };
reg_t FP_18 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 18, .name = "$f18",   .alias = "fp18" };
reg_t FP_19 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 19, .name = "$f19",   .alias = "fp19" };
reg_t FP_20 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 20, .name = "$f20",   .alias = "fp20" };
reg_t FP_21 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 21, .name = "$f21",   .alias = "fp21" };
reg_t FP_22 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 22, .name = "$f22",   .alias = "fp22" };
reg_t FP_23 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 23, .name = "$f23",   .alias = "fp23" };
reg_t FP_24 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 24, .name = "$f24",   .alias = "fp24" };
reg_t FP_25 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 25, .name = "$f25",   .alias = "fp25" };
reg_t FP_26 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 26, .name = "$f26",   .alias = "fp26" };
reg_t FP_27 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 27, .name = "$f27",   .alias = "fp27" };
reg_t FP_28 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 28, .name = "$f28",   .alias = "fp28" };
reg_t FP_29 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 29, .name = "$f29",   .alias = "fp29" };
reg_t FP_30 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 30, .name = "$f30",   .alias = "fp30" };
reg_t FP_31 = {.type = REG_PHYSICAL, .is = REG_FLOAT,        .r = 31, .name = "$f31",   .alias = "fp31" };

void init_reg() {
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

    arch_fp[0] = &FP_0;
    arch_fp[1] = &FP_1;
    arch_fp[2] = &FP_2;
    arch_fp[3] = &FP_3;
    arch_fp[4] = &FP_4;
    arch_fp[5] = &FP_5;
    arch_fp[6] = &FP_6;
    arch_fp[7] = &FP_7;
    arch_fp[8] = &FP_8;
    arch_fp[9] = &FP_9;
    arch_fp[10] = &FP_10;
    arch_fp[11] = &FP_11;
    arch_fp[12] = &FP_12;
    arch_fp[13] = &FP_13;
    arch_fp[14] = &FP_14;
    arch_fp[15] = &FP_15;
    arch_fp[16] = &FP_16;
    arch_fp[17] = &FP_17;
    arch_fp[18] = &FP_18;
    arch_fp[19] = &FP_19;
    arch_fp[20] = &FP_20;
    arch_fp[21] = &FP_21;
    arch_fp[22] = &FP_22;
    arch_fp[23] = &FP_23;
    arch_fp[24] = &FP_24;
    arch_fp[25] = &FP_25;
    arch_fp[26] = &FP_26;
    arch_fp[27] = &FP_27;
    arch_fp[28] = &FP_28;
    arch_fp[29] = &FP_29;
    arch_fp[30] = &FP_30;
    arch_fp[31] = &FP_31;

    free_all_registers();
}

void free_all_registers() {
    int i;

    for(i=0;i<ARCH_REG_CONTENT_NUM;i++) {
        reg_pool_temp[i] = arch[i+8];
        reg_pool_content[i] = arch[i+16];
    }

    reg_pool_temp[8] = arch[24];
    reg_pool_temp[9] = arch[25];

    for(i=0;i<ARCH_FP_REG_NUM;i++) {
        reg_pool_float[i] = arch_fp[i];
    }
}

reg_t *get_available_reg(reg_type_t type) {
    int i;

    reg_t *tmp;

    if (type==REG_CONTENT) {
        for(i=0;i<ARCH_REG_CONTENT_NUM;i++) {
            tmp = reg_pool_content[i];
            if (tmp) {
                reg_pool_content[i] = NULL;
                return tmp;
            }
        }
    } else if (type==REG_TEMP) {
        for(i=0;i<ARCH_REG_TEMP_NUM;i++) {
            tmp = reg_pool_temp[i];
            if (tmp) {
                reg_pool_temp[i] = NULL;
                return tmp;
            }
        }
    } else if (type==REG_FLOAT) {
        for(i=0;i<ARCH_FP_REG_NUM;i++) {
            tmp = reg_pool_float[i];
            if (tmp) {
                reg_pool_float[i] = NULL;
                return tmp;
            }
        }
    }

    return NULL;
}

void release_reg(reg_t *reg) {
    if (!reg) {
        return;
    }

    if (reg->is==REG_CONTENT) {
        reg_pool_content[reg->r] = reg;
    } else if (reg->is==REG_TEMP) {
        reg_pool_temp[reg->r] = reg;
    } else if (reg->is==REG_FLOAT) {
        reg_pool_float[reg->r] = reg;
    }
}
