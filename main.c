////
//
// Copyright (c) 2010- 
//      NAKASATO, Naohito
//      All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <cblas.h>
#include "accllib.h"

#include "kernel_TN.h"
#include "kernel_NN.h"
#include "kernel_NT.h"
#include "kernel_TT.h"

int dev = 0;

double e_time(void)
{
  static struct timeval now;
  gettimeofday(&now, NULL);
  return (double)(now.tv_sec  + now.tv_usec/1000000.0);
}

double *tmp_A, *tmp_B, *tmp_C, *tmp_D, *tmp_E;

void get_matrix(int dev, double *src, int isrc, double *dma, int idma, int s)
{
  ACCLmemcpy(dev, isrc, idma);
  memcpy(src, dma, s);
}

void set_matrix(int dev, double *src, int isrc, double *dma, int idma, int s)
{
  memcpy(dma, src, s);
  ACCLmemcpy(dev, idma, isrc);
}

void my_dgemm(int dev, int mod, int n)
{
  set_matrix(dev, tmp_A, 0, tmp_E, 6, sizeof(double)*n*n);
  set_matrix(dev, tmp_B, 1, tmp_E, 6, sizeof(double)*n*n);
  set_matrix(dev, tmp_C, 2, tmp_E, 6, sizeof(double)*n*n);

  ACCLrun_on_domain(dev, mod, n/4, n/4);

  get_matrix(dev, tmp_C, 2, tmp_E, 6, sizeof(double)*n*n);
}

void check(double *p1, double *p2, int n)
{
  double norm = 0.0;
  for(int i = 0; i < n*n; i++) {
    double dx = fabs(p1[i] - p2[i]);
    if (dx > norm) {
      norm = dx;
    }
  }
  printf("N = %5i : norm %e\t", n, norm);
}

void out(int n, double t_noio, double t_io, int niter)
{
  fprintf(stdout, "%g %g %g %g\n",
	  t_noio/niter, niter*(double)n*(double)n*(double)n*2.0/t_noio/1.0e9, 
	  t_io/niter,   niter*(double)n*(double)n*(double)n*2.0/t_io/1.0e9
	  );
}

int main(int narg, char **argv)
{
  int n;
  int ss;
  int flag = 0;
  int test_mode = 0;
  int mod;
  double alpha, beta;

  ACCLopen();
  ACCLallocate(dev);

  if ( ACCLloadkernel(dev, 0, d_TN, 2) < 0) exit(-1);
  if ( ACCLloadkernel(dev, 1, d_NN, 2) < 0) exit(-1);
  if ( ACCLloadkernel(dev, 2, d_NT, 2) < 0) exit(-1);
  if ( ACCLloadkernel(dev, 3, d_TT, 2) < 0) exit(-1);

  if (narg == 1) {
    n = 512;
  } else {
    n = atoi(argv[1]);
  }
  if (n % 64 != 0) {
    printf("N is not multiple of 64! Error!\n");
    exit(-1);
  }
  if (narg >= 3) {
    test_mode = atoi(argv[2]);
    switch(test_mode) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    default:
      printf("Error!!!!\n");
      exit(-1);
    }
  }
  if (narg == 4) flag = 1;

  alpha = 1.0;
  beta = 1.0;

  ss = n*n;

  mod = test_mode;
  ACCLallocatememory(dev, mod, 0, ACCL_DOUBLE_2_2DIM_GPU, n/2, n);
  ACCLallocatememory(dev, mod, 1, ACCL_DOUBLE_2_2DIM_GPU, n/2, n);
  ACCLallocatememory(dev, mod, 2, ACCL_DOUBLE_2_2DIM_GPU | ACCL_GLOBAL_BUFFER, n/2, n);
  ACCLallocateresource0(dev, 3, ACCL_FLOAT_4_1DIM_WRITE, 1, 1);
  ACCLrebindmemory(dev, mod, 3, "cb0");
  ACCLallocateresource0(dev, 4, ACCL_INT_4_1DIM_WRITE, 1, 1);
  ACCLrebindmemory(dev, mod, 4, "cb1");
  ACCLallocateresource0(dev, 5, ACCL_DOUBLE_2_1DIM_WRITE, 1, 1);
  ACCLrebindmemory(dev, mod, 5, "cb2");

  srand(time(NULL));
  tmp_A = (double *)memalign(256, ss*sizeof(double));
  tmp_B = (double *)memalign(256, ss*sizeof(double));
  tmp_C = (double *)memalign(256, ss*sizeof(double));
  tmp_D = (double *)memalign(256, ss*sizeof(double));
  tmp_E = (double *)memalign(256, ss*sizeof(double));
  ACCLallocateresource_pinned(dev, 6, ACCL_DOUBLE_2_2DIM_CPU, n/2, n, tmp_E, ss*sizeof(double));

  for(int i = 0; i < ss; i++) {
    tmp_A[i] = (double)rand()/(RAND_MAX);
    tmp_B[i] = (double)rand()/(RAND_MAX);
    tmp_C[i] = (double)rand()/(RAND_MAX);
  }

  if (flag == 1) {
    memcpy(tmp_D, tmp_C, ss*sizeof(double));

    switch(test_mode) {
    case 0:
      cblas_dgemm(CblasRowMajor, CblasTrans, CblasNoTrans, n, n, n, alpha, tmp_A, n, tmp_B, n, beta, tmp_D, n);
      break;
    case 1:
      cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, n, n, n, alpha, tmp_A, n, tmp_B, n, beta, tmp_D, n);
      break;
    case 2:
      cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasTrans, n, n, n, alpha, tmp_A, n, tmp_B, n, beta, tmp_D, n);
      break;
    case 3:
      cblas_dgemm(CblasRowMajor, CblasTrans, CblasTrans, n, n, n, alpha, tmp_A, n, tmp_B, n, beta, tmp_D, n);
      break;
    }
  }

  float cb[4] = {(float)(n/2), (float)n, 0.0, 0.0};
  ACCLwritememory_float(dev, 3, 4, cb);
  int cb1[4] = {0, n/2, n, 3*n/2};
  ACCLwritememory_int(dev, 4, 4, cb1);
  double cb2[2] = {beta, alpha};
  ACCLwritememory_double(dev, 5, 2, cb2);

  my_dgemm(dev, mod, n);

  if (flag == 1) {
    check(tmp_D, tmp_C, n);
  } 

  {
    const int ccc = 5;
    double dum = e_time();
    for(int i = 0; i < ccc; i++) {
      ACCLrun_on_domain(dev, mod, n/4, n/4);
    }
    dum = e_time() - dum;

    double dum2 = e_time();
    for(int i = 0; i < ccc; i++) my_dgemm(dev, mod, n);
    dum2 = e_time() - dum2;

    out(n, dum, dum2, ccc);
  }

  ACCLcleanup(dev);

  return 0;
}
