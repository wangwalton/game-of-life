#include <math.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/life.h"
#include "../src/load.h"
#include "../src/save.h"


void print_board(char* board, int nrows, int ncols) {
    printf("Printing board:\n");
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            printf("%d", board[row * ncols + col]);
        }
        printf("\n");
    }
    printf("\n");
}

char*  load_board_fname(char* filename, int* nrows, int* ncols) {
    FILE* input = fopen (filename, "r");
    if (input == NULL)
    {
        fprintf (stderr, "*** Failed to open file \'%s\' for reading! ***\n", filename);
        exit (EXIT_FAILURE);
    }
    char* temp = load_board(input, nrows, ncols);
    fclose(input);
    return temp;
}

void test_same_board() {
    int nrows = 64;
    int ncols = 64;
    char input_board[] = "tst/data/64.pbm";
    char output_board[] = "tst/data/64.pbm";
    
    char* inboard = load_board_fname (input_board, &nrows, &ncols);
    char* outboard = load_board_fname (output_board, &nrows, &ncols);
    char res = same_subboard(inboard, outboard, 0, 64, 0, 64, 64);
    assert(res == 1);
    printf("Passed test_same_board!\n");
}

void test_diff_board() {
    int nrows = 64;
    int ncols = 64;
    char input_board[] = "tst/data/64.pbm";
    char output_board[] = "tst/data/64_2.pbm";
    
    char* inboard = load_board_fname (input_board, &nrows, &ncols);
    char* outboard = load_board_fname (output_board, &nrows, &ncols);
    char res = same_subboard(inboard, outboard, 0, 64, 0, 64, 64);
    assert(res == 0);
    printf("Passed test_diff_board!\n");
}

void test_diff_board_2() {
    int nrows = 64;
    int ncols = 64;
    char input_board[] = "tst/data/64.pbm";
    char output_board[] = "tst/data/64_2.pbm";
    
    char* inboard = load_board_fname (input_board, &nrows, &ncols);
    char* outboard = load_board_fname (output_board, &nrows, &ncols);
    char res = same_subboard(inboard, outboard, 32, 60, 32, 55, 64);
    assert(res == 0);
    printf("Passed test_diff_board_2!\n");
}

void test_copy_board_2() {
    int nrows = 64;
    int ncols = 64;
    char input_board[] = "tst/data/64.pbm";
    char output_board[] = "tst/data/64_2.pbm";
    
    
    char* inboard = load_board_fname (input_board, &nrows, &ncols);
    char* outboard = load_board_fname (output_board, &nrows, &ncols);

    copy_subboard(inboard, outboard, 32, 60, 32, 55, 64);

    char res = same_subboard(inboard, outboard, 32, 60, 32, 55, 64);
    assert(res == 1);
    printf("Passed test_copy_board_2!\n");
}

void test_copy_board() {
    int nrows = 64;
    int ncols = 64;
    char input_board[] = "tst/data/64.pbm";
    char output_board[] = "tst/data/64_2.pbm";
    
    
    char* inboard = load_board_fname (input_board, &nrows, &ncols);
    char* outboard = load_board_fname (output_board, &nrows, &ncols);

    copy_subboard(inboard, outboard, 0, 64, 0, 64, 64);

    char res = same_subboard(inboard, outboard, 0, 64, 0, 64, 64);
    assert(res == 1);
    printf("Passed test_copy_board!\n");
}


int 
main (int argc, char* argv[]) {
    test_same_board();
    test_diff_board();
    test_diff_board_2();
    test_copy_board();
    test_copy_board_2();
}