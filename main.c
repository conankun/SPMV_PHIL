/**
 *  \file tests/matmult.c
 *  \brief Test sparse matrix-vector multiply.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <oski/oski.h>

#define MAX_LEN 250

struct NNZ {
  int r, c;
  double val;
};

struct COO {
  int m, n, nnz;
  struct NNZ *data;
};

struct CSR {
  int m, n, nnz;
  oski_index_t *Aptr;
  oski_index_t *Aind;
  oski_value_t *Aval;
};

/*
 *  User's initial data:
 *
 *       /  1     0     0  \          / .25 \            / 1 \
 *  A = |  -2     1     0   | ,  x = |  .45  |  , y_0 = |  1  |
 *       \  .5    0     1  /          \ .65 /            \ 1 /
 *
 *  A is a sparse lower triangular matrix with a unit diagonal,
 *  stored implicitly. This representation uses 0-based indices.
 */

static struct COO* readMatrix(char* filename) {
  struct COO *coo;
  int i;

  coo = (struct COO *)malloc(sizeof(struct COO));

  // Read file
  FILE *f = fopen(filename, "r");

  if(!f) {
    free(coo);
    return NULL;
  }

  fscanf(f, "%d%d%d",&coo->m,&coo->n,&coo->nnz);

  coo->data = (struct NNZ *)malloc(sizeof(struct NNZ) * coo->nnz);

  for(i=0;i<coo->nnz;i++) {
      fscanf(f, "%d%d%lf", &coo->data[i].r, &coo->data[i].c, &coo->data[i].val);
  }
  fclose(f);

  return coo;
}

static struct CSR* Coo2Csr(struct COO *coo) {
  return NULL;
}

static oski_value_t alpha = -1, beta = 1;

/* Solution vector */
static oski_value_t y_true[] = { .75, 1.05, .225 };

/* ----------------------------------------------------------------- */
static oski_matrix_t create_matrix (oski_index_t *Aptr, oski_index_t *Aind, oski_value_t *Aval) {
  oski_matrix_t A_tunable = oski_CreateMatCSR (Aptr, Aind, Aval, 3, 3,	/* CSR arrays */
					       SHARE_INPUTMAT,	/* Copy mode */
					       /* non-zero pattern semantics */
					       3, INDEX_ZERO_BASED,
					       MAT_GENERAL,
					       MAT_DIAG_EXPLICIT);

  if (A_tunable == INVALID_MAT)
    exit (1);

  return A_tunable;
}

static oski_vecview_t create_x (oski_value_t *x)
{
  oski_vecview_t x_view = oski_CreateVecView (x, 3, STRIDE_UNIT);
  if (x_view == INVALID_VEC)
    exit (1);
  return x_view;
}

static oski_vecview_t create_y (oski_value_t *y)
{
  oski_vecview_t y_view = oski_CreateVecView (y, 3, STRIDE_UNIT);
  if (y_view == INVALID_VEC)
    exit (1);
  return y_view;
}

static void run (int r, int c, int operation) {
  int err;


  /* Solution */
  char ans_buffer[128], true_buffer[128];

  //sprintf (true_buffer, "[ %.3f ; %.3f ; %.3f ]", y_true[0], y_true[1], y_true[2]);
  //printf ("Answer should be: '%s'\n", true_buffer);

  /* Perform matrix vector multiply */
  
  double total_time = 0;

  while(operation--) {
  
    oski_index_t Aptr[] = { 0, 1, 3, 5 };
    oski_index_t Aind[] = { 0, 0, 1, 0, 2 };
    oski_value_t Aval[] = { 1, -2, 1, 0.5, 1 };
    oski_value_t x[] = { .25, .45, .65 };
    oski_value_t y[] = { 1, 1, 1 };

    clock_t begin = clock();

    /* Create a tunable sparse matrix object. */
    oski_matrix_t A_tunable = create_matrix (Aptr, Aind, Aval);
    oski_vecview_t x_view = create_x (x);
    oski_vecview_t y_view = create_y (y);

    // Explicit Turning
    oski_SetHintMatMult(A_tunable, OP_NORMAL, 1.0, SYMBOLIC_VEC, 1.0, SYMBOLIC_VEC, operation);
    oski_SetHint(A_tunable, HINT_SINGLE_BLOCKSIZE, r, c);
    oski_TuneMat(A_tunable);
    
    // Multiply Matrix
    err = oski_MatMult (A_tunable, OP_NORMAL, alpha, x_view, beta, y_view);
    if (err)
      exit (1);
    
    oski_DestroyMat (A_tunable);
    oski_DestroyVecView (x_view);
    oski_DestroyVecView (y_view);
  
    clock_t end = clock();
    
    total_time += (double)(end-begin)/CLOCKS_PER_SEC;
    
    /* Print result, y. Should be "[ 0.750 ; 1.050 ; 0.225 ]" */
    if(operation == 0) {
      sprintf (ans_buffer, "[ %.3f ; %.3f ; %.3f ]", y[0], y[1], y[2]);
      printf ("Returned: '%s'\n", ans_buffer);
    }
  }


  printf ("Elapsed Time (Matmult): %lf seconds\n", total_time);
  
  
}

int main (int argc, char *argv[])
{
  int i,j;
  
  char filename[MAX_LEN];
  printf("Enter the filename of the matrix: ");
  scanf("%s", filename);

  struct COO *coo = readMatrix(filename);
  if(!coo) {
    printf("File does not exist\n");
    return 1;
  }
  struct CSR *csr = Coo2Csr(coo);
  /* Initialize library; will happen automatically eventually ... */

  if (!oski_Init ())
    return 1;
  for(i=1;i<=16;i++) {
    for(j=1;j<=16;j++) {
      printf("======== Block Size : %d x %d ========\n\n", i, j);
      run(i,j, 1);
      printf("=========================================\n\n");
    }
  }

  oski_Close ();
  return 0;
}

/* eof */
