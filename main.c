/**
 *  \file tests/matmult.c
 *  \brief Test sparse matrix-vector multiply.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <oski/oski.h>


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
static oski_index_t Aptr[] = { 0, 0, 1, 2 };
static oski_index_t Aind[] = { 0, 0 };
static oski_value_t Aval[] = { -2, 0.5 };
static oski_value_t x[] = { .25, .45, .65 };
static oski_value_t y[] = { 1, 1, 1 };

static oski_value_t alpha = -1, beta = 1;

/* Solution vector */
static oski_value_t y_true[] = { .75, 1.05, .225 };

/* ----------------------------------------------------------------- */
static oski_matrix_t create_matrix (void) {
  oski_matrix_t A_tunable = oski_CreateMatCSR (Aptr, Aind, Aval, 3, 3,	/* CSR arrays */
					       SHARE_INPUTMAT,	/* Copy mode */
					       /* non-zero pattern semantics */
					       3, INDEX_ZERO_BASED,
					       MAT_TRI_LOWER,
					       MAT_UNIT_DIAG_IMPLICIT);

  if (A_tunable == INVALID_MAT)
    exit (1);

  return A_tunable;
}

static oski_vecview_t create_x (void)
{
  oski_vecview_t x_view = oski_CreateVecView (x, 3, STRIDE_UNIT);
  if (x_view == INVALID_VEC)
    exit (1);
  return x_view;
}

static oski_vecview_t create_x_short (void)
{
  oski_vecview_t x_view = oski_CreateVecView (x, 2, STRIDE_UNIT);
  if (x_view == INVALID_VEC)
    exit (1);
  return x_view;
}

static oski_vecview_t create_y (void)
{
  oski_vecview_t y_view = oski_CreateVecView (y, 3, STRIDE_UNIT);
  if (y_view == INVALID_VEC)
    exit (1);
  return y_view;
}

static void run (void) {
  int err;

  /* Create a tunable sparse matrix object. */
  oski_matrix_t A_tunable = create_matrix ();
  oski_vecview_t x_view = create_x ();
  oski_vecview_t y_view = create_y ();

  /* Solution */
  char ans_buffer[128], true_buffer[128];

  sprintf (true_buffer, "[ %.3f ; %.3f ; %.3f ]",
	   y_true[0], y_true[1], y_true[2]);
  printf ("Answer should be: '%s'\n", true_buffer);

  clock_t begin = clock();
  /* Perform matrix vector multiply */
  err = oski_MatMult (A_tunable, OP_NORMAL, alpha, x_view, beta, y_view);
  if (err)
    exit (1);

  oski_DestroyMat (A_tunable);
  oski_DestroyVecView (x_view);
  oski_DestroyVecView (y_view);
  clock_t end = clock();
  /* Print result, y. Should be "[ 0.750 ; 1.050 ; 0.225 ]" */
  sprintf (ans_buffer, "[ %.3f ; %.3f ; %.3f ]", y[0], y[1], y[2]);
  
  printf ("Elapsed Time (Matmult): %lf seconds\n", (double)(end-begin)/CLOCKS_PER_SEC);
  printf ("Returned: '%s'\n", ans_buffer);
  
  if (strcmp (ans_buffer, true_buffer) != 0)
    {
      fprintf (stderr, "*** Got the wrong answer! ***\n");
      exit (1);
    }
}

int main (int argc, char *argv[])
{
  /* Initialize library; will happen automatically eventually ... */
  if (!oski_Init ())
    return 1;

  run();

  oski_Close ();
  return 0;
}

/* eof */
