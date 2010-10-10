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
#include "cal.h"

#define ACCL_FLOAT_1_1DIM_WRITE (CAL_FORMAT_FLOAT_1 | (0x1 << 10) | (0x1 << 20))
#define ACCL_FLOAT_2_1DIM_WRITE (CAL_FORMAT_FLOAT_2 | (0x1 << 10) | (0x1 << 20))
#define ACCL_FLOAT_4_1DIM_WRITE (CAL_FORMAT_FLOAT_4 | (0x1 << 10) | (0x1 << 20))
#define ACCL_FLOAT_1_2DIM_WRITE (CAL_FORMAT_FLOAT_1 | (0x1 << 11) | (0x1 << 20))
#define ACCL_FLOAT_2_2DIM_WRITE (CAL_FORMAT_FLOAT_2 | (0x1 << 11) | (0x1 << 20))
#define ACCL_FLOAT_4_2DIM_WRITE (CAL_FORMAT_FLOAT_4 | (0x1 << 11) | (0x1 << 20))

#define ACCL_FLOAT_1_1DIM_READ  (CAL_FORMAT_FLOAT_1 | (0x1 << 10) | (0x1 << 21))
#define ACCL_FLOAT_2_1DIM_READ  (CAL_FORMAT_FLOAT_2 | (0x1 << 10) | (0x1 << 21))
#define ACCL_FLOAT_4_1DIM_READ  (CAL_FORMAT_FLOAT_4 | (0x1 << 10) | (0x1 << 21))
#define ACCL_FLOAT_1_2DIM_READ  (CAL_FORMAT_FLOAT_1 | (0x1 << 11) | (0x1 << 21))
#define ACCL_FLOAT_2_2DIM_READ  (CAL_FORMAT_FLOAT_2 | (0x1 << 11) | (0x1 << 21))
#define ACCL_FLOAT_4_2DIM_READ  (CAL_FORMAT_FLOAT_4 | (0x1 << 11) | (0x1 << 21))

#define ACCL_INT_1_1DIM_WRITE (CAL_FORMAT_INT_1 | (0x1 << 10) | (0x1 << 20))
#define ACCL_INT_2_1DIM_WRITE (CAL_FORMAT_INT_2 | (0x1 << 10) | (0x1 << 20))
#define ACCL_INT_4_1DIM_WRITE (CAL_FORMAT_INT_4 | (0x1 << 10) | (0x1 << 20))
#define ACCL_INT_1_2DIM_WRITE (CAL_FORMAT_INT_1 | (0x1 << 11) | (0x1 << 20))
#define ACCL_INT_2_2DIM_WRITE (CAL_FORMAT_INT_2 | (0x1 << 11) | (0x1 << 20))
#define ACCL_INT_4_2DIM_WRITE (CAL_FORMAT_INT_4 | (0x1 << 11) | (0x1 << 20))

#define ACCL_INT_1_1DIM_READ  (CAL_FORMAT_INT_1 | (0x1 << 10) | (0x1 << 21))
#define ACCL_INT_2_1DIM_READ  (CAL_FORMAT_INT_2 | (0x1 << 10) | (0x1 << 21))
#define ACCL_INT_4_1DIM_READ  (CAL_FORMAT_INT_4 | (0x1 << 10) | (0x1 << 21))
#define ACCL_INT_1_2DIM_READ  (CAL_FORMAT_INT_1 | (0x1 << 11) | (0x1 << 21))
#define ACCL_INT_2_2DIM_READ  (CAL_FORMAT_INT_2 | (0x1 << 11) | (0x1 << 21))
#define ACCL_INT_4_2DIM_READ  (CAL_FORMAT_INT_4 | (0x1 << 11) | (0x1 << 21))

#define ACCL_DOUBLE_1_1DIM_WRITE (CAL_FORMAT_UNSIGNED_INT32_2 | (0x1 << 10) | (0x1 << 20))
#define ACCL_DOUBLE_2_1DIM_WRITE (CAL_FORMAT_UNSIGNED_INT32_4 | (0x1 << 10) | (0x1 << 20))
#define ACCL_DOUBLE_1_1DIM_READ  (CAL_FORMAT_UNSIGNED_INT32_2 | (0x1 << 10) | (0x1 << 21))
#define ACCL_DOUBLE_2_1DIM_READ  (CAL_FORMAT_UNSIGNED_INT32_4 | (0x1 << 10) | (0x1 << 21))

#define ACCL_DOUBLE_1_2DIM_WRITE (CAL_FORMAT_UNSIGNED_INT32_2 | (0x1 << 11) | (0x1 << 20))
#define ACCL_DOUBLE_2_2DIM_WRITE (CAL_FORMAT_UNSIGNED_INT32_4 | (0x1 << 11) | (0x1 << 20))
#define ACCL_DOUBLE_1_2DIM_READ  (CAL_FORMAT_UNSIGNED_INT32_2 | (0x1 << 11) | (0x1 << 21))
#define ACCL_DOUBLE_2_2DIM_READ  (CAL_FORMAT_UNSIGNED_INT32_4 | (0x1 << 11) | (0x1 << 21))

#define ACCL_DOUBLE_1_2DIM_GPU (CAL_FORMAT_UNSIGNED_INT32_2 | (0x1 << 11) | (0x1 << 20))
#define ACCL_DOUBLE_2_2DIM_GPU (CAL_FORMAT_UNSIGNED_INT32_4 | (0x1 << 11) | (0x1 << 20))
#define ACCL_DOUBLE_1_2DIM_CPU (CAL_FORMAT_UNSIGNED_INT32_2 | (0x1 << 11) | (0x1 << 21))
#define ACCL_DOUBLE_2_2DIM_CPU (CAL_FORMAT_UNSIGNED_INT32_4 | (0x1 << 11) | (0x1 << 21))

#define ACCL_FLOAT_4_2DIM_GPU (CAL_FORMAT_FLOAT_4 | (0x1 << 11) | (0x1 << 20))
#define ACCL_FLOAT_4_2DIM_CPU (CAL_FORMAT_FLOAT_4 | (0x1 << 11) | (0x1 << 21))


#define ACCL_GLOBAL_BUFFER (0x1<<22)

#ifdef __cplusplus
extern "C" {
#endif

int ACCLopen(void);
int ACCLallocate(int idev);
int ACCLgettarget(int idev);
void ACCLcleanup(int idev);
int ACCLloadkernel(int idev, int imod, const char *filename, int text);
int ACCLallocatememory(int idev, int imod, int i, int type, int nx, int ny);
void ACCLreleasememories(int idev);
void ACCLdumpresources(int idev);
void ACCLrun_on_domain(int idev, int imod, int w, int h);
int ACCLbindmemory(int idev, int i, int imod);
void ACCLallocateresource(int idev, int i, int type, int nx, int ny);
int ACCLrebindmemory(int idev, int imod, int i, const char *vn);
int ACCLchangemodbinding(int idev, int imod, int i);
void ACCLmemcpy(int idev, int src, int dst);
void ACCLallocateresource0(int idev, int i, int type, int nx, int ny);
void ACCLreleasememory(int idev, int i);

int ACCLwritememory(int idev, int i, int size, void *);
int ACCLreadmemory(int idev, int i, int size, void *);

int ACCLwritememory_double(int idev, int i, int n, double *p);
int ACCLreadmemory_double(int idev, int i, int n, double *p);
int ACCLwritememory_float(int idev, int i, int n, float *p);
int ACCLreadmemory_float(int idev, int i, int n, float *p);
int ACCLwritememory_int(int idev, int i, int n, int *p);
int ACCLreadmemory_int(int idev, int i, int n, int *p);

void ACCLallocateresource_pinned(int idev, int i, int type, int nx, int ny, void *p, int size);

// QD
#ifdef QD
int ACCLwritememory_dd(int idev, int i, int size, dd_real *mp);
int ACCLreadmemory_dd(int idev, int i, int size, dd_real *mp);
#endif

// special functions
int ACCLwritememory_batch(int idev, int n, int *il, int *sl, void **p_src);
CALresource ACCL_get_res(int idev, int i);
int ACCLrebindmemory0(int idev, int imod, int i, const char *vn);
void ACCLdumpimage(int idev, int imod);

void ACCLmemcpy0(int idev, int src, int dst);
void ACCLrun_on_domain0(int idev, int imod, int w, int h);
void ACCLrun_on_domain2(int idev, int imod, int x0, int y0, int x1, int y1);
void ACCLwait(int idev);
void ACCLctxflush(int idev);
void ACCLwait0(int idev, int src);
#ifdef __cplusplus
  }
#endif

void ACCLsetupcounter(int);
void ACCL_start_c_idle(int);
void ACCL_stop_c_idle(int);
void ACCL_start_c_cache(int);
void ACCL_stop_c_cache(int);
float ACCL_c_idle(int);
float ACCL_c_cache(int);
