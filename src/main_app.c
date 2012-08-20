#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "build_flags.h"
#include "opt_for_stmt.h"
#include "main_app.h"

#if LEX_MAIN == 0

void print_version() {
    printf("%s version %d.%d\n",PROGRAM_NAME,PROGRAM_MAJOR_VERSION,PROGRAM_MINOR_VERSION);
    printf("Latest version can be found at: https://github.com/kapamaroo/ladybug\n\n");
}

void print_author() {
    printf("AUTHOR:\t Maroudas Manolis ");
    printf("<kapamaroo@gmail.com>\n\n");
}

void print_extra() {
    printf("See TODO for what is on the way.\n");
    printf("Feel free to change whatever you like :)\n");
    printf("\n");
}

void help() {
    print_version();
    print_author();

    printf("Usage: ladybug [OPTIONS] SRC_FILE\n");
    printf("\nOPTIONS\n");
    printf("\t --lunroll-classic\t enable classic loop unrolling (copy the body multiple times)\n");
    printf("\t --lunroll-symbolic\t enable software pipelining aka symbolic loop unroll\n");
    printf("\t --lsimplify\t\t move independent statements out of loop UNSAFE (disabled for your safety)\n");
    printf("\t --help\t\t print this help message and exit\n");
    printf("\n");

    print_extra();
}

int enable_opt_loop_unroll_classic = 0;
int enable_opt_loop_unroll_symbolic = 0;
int enable_opt_loop_simplify = 0;

char *src_input;

void parse_args(int argc, char *argv[]) {
    int i;

    src_input = NULL;

    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i],"--lunroll-classic"))
            enable_opt_loop_unroll_classic = 1;

        else if (!strcmp(argv[i],"--lunroll-symbolic"))
            enable_opt_loop_unroll_symbolic = 1;

        else if (!strcmp(argv[i],"--lsimplify"))
            enable_opt_loop_simplify = 1;

        else if (!strcmp(argv[i],"--help")) {
            help();
            exit(EXIT_SUCCESS);
        }

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
