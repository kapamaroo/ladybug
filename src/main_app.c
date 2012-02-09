#include <stdio.h>
#include "main_app.h"

#if LEX_MAIN == 0

void help() {
    printf("%s version %d.%d\n",PROGRAM_NAME,PROGRAM_MAJOR_VERSION,PROGRAM_MINOR_VERSION);
    printf("Usage: The program takes one parameter, the source code file.\nexample: ladybug src.p\n");
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
