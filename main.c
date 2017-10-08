/**
 *  \file tests/matmult.c
 *  \brief Test sparse matrix-vector multiply.
 */

// TODO: implement freeing matrix/vector from memory

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <oski/oski.h>

#define MAX_LEN 250

struct NNZ {
  oski_index_t r, c;
  oski_value_t val;
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

struct VECTOR {
  oski_value_t *x;
  oski_vecview_t *x_view;
};


// Function Declaration
static struct COO* readMatrix(char* filename);
static struct CSR* Coo2Csr(struct COO *coo);
static oski_matrix_t create_matrix (struct CSR *csr);
static struct VECTOR* create_x (int size);
static void run (struct CSR *csr, int r, int c, int operation);
static void displayCSR(struct CSR *csr);

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
  // TODO: check if matrix is sorted by row (and column if same row) and sort if not
  return coo;
}

static struct CSR* Coo2Csr(struct COO *coo) {
  struct CSR *csr = (struct CSR *)malloc(sizeof(struct CSR));
  int i;

  // Copy dimensions, nnz
  csr->m = coo->m;
  csr->n = coo->n;
  csr->nnz = coo->nnz;
  
  // Memory Allocation
  csr->Aptr = (oski_index_t *)calloc(csr->m+1, sizeof(oski_index_t));
  csr->Aind = (oski_index_t *)malloc(sizeof(oski_index_t) * csr->nnz);
  csr->Aval = (oski_value_t *)malloc(sizeof(oski_value_t) * csr->nnz);

  // conversion COO -> CSR
  for(i = 0; i < csr->nnz; i++) {
    csr->Aptr[coo->data[i].r]++;
    csr->Aind[i] = coo->data[i].c-1; // since column index is starting from 0
    csr->Aval[i] = coo->data[i].val;
  }

  // sum over Aptr
  for(i = 1; i <= csr->m; i++) {
    csr->Aptr[i] += csr->Aptr[i-1];
  }

  return csr;
}

static oski_value_t alpha = -1, beta = 1;

/* Solution vector */
static oski_value_t y_true[] = { .75, 1.05, .225 };

/* ----------------------------------------------------------------- */
static oski_matrix_t create_matrix (struct CSR *csr) {
  oski_matrix_t A_tunable = oski_CreateMatCSR (csr->Aptr, csr->Aind, csr->Aval, csr->m, csr->n,	/* CSR arrays */
					       SHARE_INPUTMAT,	/* Copy mode */
					       /* non-zero pattern semantics */
					       3, INDEX_ZERO_BASED,
					       MAT_GENERAL,
					       MAT_DIAG_EXPLICIT);
  
  if (A_tunable == INVALID_MAT) {
    printf("Matrix provided is invalid.\n");
    exit (1);
  }
  
  return A_tunable;
}

static struct VECTOR* create_x (int size)
{
  int i;
  oski_value_t *x = (oski_value_t *)malloc(sizeof(oski_value_t) * size);
  oski_vecview_t *x_view = (oski_vecview_t *)malloc(sizeof(oski_vecview_t));
  // our range will be 0 < x[i] < 1
  for(i = 0; i < size; i++) {
    x[i] = rand()%100 / 100.;
  }
  *x_view = oski_CreateVecView (x, size, STRIDE_UNIT);
  if (*x_view == INVALID_VEC)
    exit (1);
  struct VECTOR *vec = (struct VECTOR *)malloc(sizeof(struct VECTOR));
  vec->x = x;
  vec->x_view = x_view;
  return vec;
}

static void run (struct CSR *csr, int r, int c, int operation) {
  int err;


  /* Solution */
  char ans_buffer[128], true_buffer[128];

  //sprintf (true_buffer, "[ %.3f ; %.3f ; %.3f ]", y_true[0], y_true[1], y_true[2]);
  //printf ("Answer should be: '%s'\n", true_buffer);

  /* Perform matrix vector multiply */
  
  double total_time = 0;

  while(operation--) {

    

    /* Create a tunable sparse matrix object. */
    oski_matrix_t A_tunable = create_matrix (csr);
    struct VECTOR *x_view_vec = create_x (csr->n);
    struct VECTOR *y_view_vec = create_x (csr->n);

    oski_vecview_t *x_view = x_view_vec->x_view;
    oski_vecview_t *y_view = y_view_vec->x_view;

    // Explicit Turning
    oski_SetHintMatMult(A_tunable, OP_NORMAL, 1.0, SYMBOLIC_VEC, 1.0, SYMBOLIC_VEC, operation);
    oski_SetHint(A_tunable, HINT_SINGLE_BLOCKSIZE, r, c);
    oski_TuneMat(A_tunable);
    
    clock_t begin = clock();
    // Multiply Matrix
    err = oski_MatMult (A_tunable, OP_NORMAL, alpha, *x_view, beta, *y_view);
    clock_t end = clock();

    if (err)
      exit (1);
    
    oski_DestroyMat (A_tunable);
    oski_DestroyVecView (*x_view);
    oski_DestroyVecView (*y_view);
  
    total_time += (double)(end-begin)/CLOCKS_PER_SEC;
    
    /* Print result, y. Should be "[ 0.750 ; 1.050 ; 0.225 ]" */
    if(operation == 0) {
      sprintf (ans_buffer, "[ %.3f ; %.3f ; %.3f ]", y_view_vec->x[0], y_view_vec->x[1], y_view_vec->x[2]);
      printf ("Returned: '%s'\n", ans_buffer);
    }
  }


  printf ("Elapsed Time (Matmult): %lf seconds\n", total_time);
  
  
}

static void displayCSR(struct CSR *csr) {
  int m = csr->m;
  int nnz = csr->nnz;
  int i;
  printf("Aptr: [");
  for(i=0;i<=m;i++) {
    printf("%d", csr->Aptr[i]);
    if(i != m) printf(", ");
  }
  printf("]\n");

  printf("Aind: [");
  for(i=0;i<nnz;i++) {
    printf("%d", csr->Aind[i]);
    if(i != nnz-1) printf(", ");
  }
  printf("]\n");

  printf("Aval: [");
  for(i=0;i<nnz;i++) {
    printf("%lf", csr->Aval[i]);
    if(i != nnz-1) printf(", ");
  }
  printf("]\n");
}

int main (int argc, char *argv[])
{
  int i,j;
  //necessary for generating random vector for SpMV
  srand(time(NULL)); 

  // get file name
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
      run(csr,i,j, 1);
      printf("=========================================\n\n");
    }
  }

  oski_Close ();
  return 0;
}

/* eof */
