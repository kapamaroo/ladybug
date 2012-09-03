#include <stdio.h>
#include <stdlib.h>  //atoi()

//pass number of loop statements
//to generate the prologue/epilogue statement pattern

void prologue(int x) {
    int i;
    int j;

    printf("//prologue\n");

    for (i=0; i<x-1; i++) {
        printf("\n//prepare iteration %d\n",i);
        for (j=0; j<x-i-1; j++)
            printf("S%d;\n",j);
    }
}

void loop(int x) {
    int i;

    printf("\n//main loop\n");
    printf("for (i=0; i<%d; i++) {\n",x);

    for (i=0; i<x; i++)
        printf("\tS%d;  //iteration %d\n",x-i-1,i);

    printf("}\n\n");
}

void epilogue(int x) {
    int i;
    int j;

    printf("//epilogue\n");

    for (i=x-2; i>=0; i--) {
        printf("\n//complete iteration %d\n",x-i-1);
        for (j=0; j<x-i-1; j++)
            printf("S%d;\n",j+i+1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2)
        return -1;

    int x = atoi(argv[1]);
    prologue(x);
    loop(x);
    epilogue(x);

    return 0;
}
