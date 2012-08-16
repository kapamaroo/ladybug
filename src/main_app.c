#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "opt_for_stmt.h"
#include "main_app.h"

#if LEX_MAIN == 0

void help() {
    printf("%s version %d.%d\n",PROGRAM_NAME,PROGRAM_MAJOR_VERSION,PROGRAM_MINOR_VERSION);
    printf("Usage: The program takes one parameter, the source code file.\nexample: ladybug src.p\n");
}

int enable_opt_loop_unroll_classic = 0;
int enable_opt_loop_unroll_symbolic = 0;
int enable_opt_loop_simplify = 0;

char *src_input;

void parse_args(int argc, char *argv[]) {
    int i;

    src_input = NULL;

    for (i=1; i<argc; i++) {
        if (!strncmp(argv[i],"--lunroll-classic",17))
            enable_opt_loop_unroll_classic = 1;

        else if (!strncmp(argv[i],"--lunroll-symbolic",18))
            enable_opt_loop_unroll_symbolic = 1;

        else if (!strncmp(argv[i],"--lsimplify",11))
            enable_opt_loop_simplify = 1;

        else if (!src_input) {
            //ignore multiple input files
            src_input = argv[i];
        }

        else
            printf("Note: Ignoring multiple arguments (%s)\n", argv[i]);
    }

    if (!src_input) {
        help();
        exit(EXIT_FAILURE);
    }
}

void compile() {
    /* Frontend @ statement_t *statement_root_module[MAX_NUM_OF_MODULES]; */
    //standard passes
    define_blocks();
    analyse_blocks();

    if (enable_opt_loop_simplify)
        optimize_loops(OPT_LOOP_SIMPLIFY);  //unsafe

    if (enable_opt_loop_unroll_classic)
        optimize_loops(OPT_UNROLL_CLASSIC);

    if (enable_opt_loop_unroll_symbolic)
        optimize_loops(OPT_UNROLL_SYMBOLIC);

    /* high level optimizations go here */



    /* EOF (end of frontend) :) */
    generate_all_modules();
    /* IR @ ir_node_t *ir_root_tree[MAX_NUM_OF_MODULES]; */



    /* IR optimizations go here */



    /* End of IR */
    parse_all_modules();
    /* Backend @ instr_t *final_tree[MAX_NUM_OF_MODULES]; */



    /* low level optimizations go here */



    /* End of Backend, Emit final code */
    print_assembly();
}

#else  /* LEX_MAIN != 0 */
int main(int argc, char* argv[]) {
    int tok_id;
    //stdout = fopen("log_flex","w");
    if (argc>1) {
        yyin = fopen(argv[argc-1],"r");
    }
    while ( (tok_id = yylex()) ) {
        printf("\treturned %d\n",tok_id);
    }
    if (argc>1) {
        fclose(yyin);
    }
    //fclose(stdout);
    return 0;
}
#endif /* LEX_MAIN != 0 */
