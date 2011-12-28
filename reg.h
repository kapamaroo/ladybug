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
} reg_type_t;

typedef struct reg_t {
    reg_type_t is;
    int r;
    char *name;
    char *alias;
} reg_t;

//extern reg_t *arch[REG_NUM];

void init_reg();
reg_t *get_available_reg(reg_type_t type);
void release_reg();

#endif
