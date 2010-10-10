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
#include <string.h>
#include <libio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cal.h"
#include "calcl.h"
#include "cal_ext.h"
#include "cal_ext_counter.h"
#include "accllib.h"

#define nmodulemax 16
#define _n 128

typedef struct ACCLobjinfo {
  char vn[_n][4];
  int nx, ny;
  CALuint type;
  int mod;
} ACCLobj;

typedef struct ACCLinstance {
  CALdevice device;
  CALdeviceattribs attribs;
  CALcontext ctx;
  CALmodule  module[nmodulemax];
  CALfunc    main_func[nmodulemax];
  CALdomain domain;
  CALevent  event;
  CALimage  image; // temporary
  //
  CALresource res[_n];
  CALevent    copy_event[_n];
  CALmem      mem[_n];
  CALname     name[_n];
  ACCLobj      obj[_n];
  // CB
  CALresource res_cb[1];
  CALmem      mem_cb[1];
  CALname     name_cb[1];
  // nj
  CALresource res_nj;
  CALmem      mem_nj;
  CALname     name_nj;
  int nmem, nout, ncb;
} ACCL;

static ACCL pp[8];
static PFNCALRESCREATE2D calResCreate2D = 0;
static PFNCALRESCREATE1D calResCreate1D = 0;
static FILE *fpdis = NULL; 

////////////////////////////////////////////////////////////////////////////
static void save_disasm(const char* msg)
{
  fprintf(fpdis, "%s", msg);
}
////////////////////////////////////////////////////////////////////////////
static void showerror(const char *s)
{
  fprintf(stderr, "%s : %s\n", s, calclGetErrorString());
  fprintf(stderr, "%s : %s\n", s, calGetErrorString());
}
////////////////////////////////////////////////////////////////////////////

int ACCLopen(void)
{
  CALuint ndev = 0;
  calInit();
  calDeviceGetCount(&ndev);
  if (ndev < 1) {
    showerror("no device is available!");
    return -1;
  }
  return ndev;
}

int ACCLallocate(int idev)
{
  ACCL *s = &pp[idev];

  if (calDeviceOpen(&(s->device), idev) != CAL_RESULT_OK) {
    showerror("calDeviceOpen");
  }

  s->attribs.struct_size = sizeof(CALdeviceattribs);
  if (calDeviceGetAttribs(&(s->attribs), idev) != CAL_RESULT_OK) {
    showerror("calDeviceGetAttribs");
  }

  calCtxCreate(&(s->ctx), s->device);

  s->nmem = 0;
  s->nout = 0;
  s->ncb  = 0;
  s->image = 0;
  for(int i = 0; i < _n; i++) s->res[i] = 0;
  for(int i = 0; i < nmodulemax; i++) s->module[i] = 0;

  return 1;
}

int ACCLgettarget(int idev)
{
  ACCL *s = &pp[idev];
  return s->attribs.target;
}

int ACCLcompilekernel(int idev, const char *filename)
{
  ACCL *s = &pp[idev];
  CALlanguage lang = CAL_LANGUAGE_IL;
  FILE *fp;
  char *prog;
  struct stat f;
  CALobject obj;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "no kernel file %s!\n", filename);
    return -1;
  }

  fstat(fileno(fp), &f);
  prog = (char *)malloc(f.st_size);
  //  printf("%ld bytes\n", f.st_size);
  fread(prog, 1, f.st_size, fp);
  fclose(fp);

  //  printf("%i\n", s->attribs.target);

  if (calclCompile(&obj, lang, prog, s->attribs.target) != CAL_RESULT_OK) {
    showerror("Program compilation failed. Exiting.");
    return -1;
  }

  if (calclLink(&s->image, &obj, 1) != CAL_RESULT_OK) {
    showerror("Program linking failed. Exiting.");
    return -1;
  }

  calclFreeObject(obj);

  return 1;
}

int ACCLcompilekernel0(int idev, const char *prog)
{
  ACCL *s = &pp[idev];
  CALlanguage lang = CAL_LANGUAGE_IL;
  CALobject obj;

  if (calclCompile(&obj, lang, prog, s->attribs.target) != CAL_RESULT_OK) {
    showerror("Program compilation failed. Exiting.");
    return -1;
  }

  if (calclLink(&s->image, &obj, 1) != CAL_RESULT_OK) {
    showerror("Program linking failed. Exiting.");
    return -1;
  }

  calclFreeObject(obj);

  return 1;
}

int ACCLwritekernel(int idev, const char *filename)
{
  ACCL *s = &pp[idev];
  CALvoid *buf;
  CALuint size;
  char ff[1000];
  FILE *fp;

  calclImageGetSize(&size, s->image);
  buf = (CALvoid *)malloc(size);
  calclImageWrite(buf, size, s->image);

  sprintf(ff, "%s.img", filename);
  fp = fopen(ff, "w");
  fwrite(buf, 1, size, fp);
  fclose(fp); 

  return 1;
}

int ACCLreadkernel(int idev, const char *filename)
{
  ACCL *s = &pp[idev];
  FILE *fp;
  struct stat f;
  CALvoid *p;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "no kernel file %s!\n", filename);
    return -1;
  }

  fstat(fileno(fp), &f);
  if (s->image != 0) {
    calclFreeImage(s->image);
    s->image = 0;
  }
  p = (CALvoid *)malloc(f.st_size);
  //  printf("%ld bytes\n", f.st_size);
  fread(p, 1, f.st_size, fp);
  fclose(fp);

  s->image = (CALimage)p;

  return 1;
}

int ACCLloadkernel(int idev, int imod, const char *filename, int text)
{
  int mode;
  ACCL *s = &pp[idev];

  if (s->module[imod] != 0) {
    fprintf(stderr, "kernel is already loaded on %i!\n", imod);
    return -1;
  }

  mode = 0xf & text;
  int ret = 0;
  switch(mode) {
  case 1: 
    ret = ACCLcompilekernel(idev, filename);
    break;
  case 0:
    ret = ACCLreadkernel(idev, filename);
    break;
  case 2:
    ret = ACCLcompilekernel0(idev, filename);
    break;
  default:
    ret = -1;
  }
  if (ret < 0) return ret;

  if (calModuleLoad(&(s->module[imod]), s->ctx, s->image) != CAL_RESULT_OK) {
    showerror("Program load failed. Exiting.");
    return -1;
  }

  if (calModuleGetEntry(&(s->main_func[imod]), s->ctx, s->module[imod], "main") != CAL_RESULT_OK) {
    showerror("GetEntry failed. Exiting.");
    return -1;
  }

  if (text > 0x100) {
  //  if (text == 1 || text == 2) {
  //  if (text == 1) {
    char buf[1000];
    sprintf(buf, "__dis__%i.txt", imod);
    fpdis = fopen(buf, "w");
    calclDisassembleImage(s->image, save_disasm);
    fclose(fpdis);
  }

  if (mode == 1) {
    ACCLwritekernel(idev, filename);
  }

  calclFreeImage(s->image);
  s->image = 0;

  return 0;
}

void ACCLdumpimage(int idev, int imod)
{
  ACCL *s = &pp[idev];
  char buf[1000];

  if (s->module[imod] == 0) {
    fprintf(stderr, "kernel is not loaded on %i!\n", imod);
    return;
  }

  if (calModuleLoad(&(s->module[imod]), s->ctx, s->image) != CAL_RESULT_OK) {
    showerror("Program load failed. Exiting.");
    return;
  }

  sprintf(buf, "__dis.txt");
  fpdis = fopen(buf, "w");
  calclDisassembleImage(s->image, save_disasm);
  fclose(fpdis);

  calclFreeImage(s->image);
  s->image = 0;
}

void ACCLunloadkernel(int idev, int imod)
{
  ACCL *s = &pp[idev];
  if (s->module[imod] == 0) return;
  calModuleUnload(s->ctx, s->module[imod]);  
}

void ACCLallocateresource0(int idev, int i, int type, int nx, int ny)
{
  CALformat c;
  CALresult result = CAL_RESULT_OK;
  int dim, rw;
  ACCL *s = &pp[idev];

  c   = (CALformat)(type & 0xff);
  dim = (type >> 10) & 0x1;
  rw  = (type >> 20) & 0x7;

  switch(rw) {
  case 1:
    {
      if (dim == 1) {
	result = calResAllocLocal1D(&(s->res[i]), s->device, nx, c, 0);    
      } else {
	result = calResAllocLocal2D(&(s->res[i]), s->device, nx, ny, c, 0);
      }
    }
    break;
  case 2:
    {
      if (dim == 1) {
	result = calResAllocRemote1D(&(s->res[i]), &(s->device), 1, nx, c, CAL_RESALLOC_CACHEABLE);
      } else {
	result = calResAllocRemote2D(&(s->res[i]), &(s->device), 1, nx, ny, c, CAL_RESALLOC_CACHEABLE);
      }
    }
    break;
  case 5:
  case 6:
    {
      // global memory on GPU memory
      if (dim == 1) {
        result = calResAllocLocal1D(&(s->res[i]), s->device, nx, c, CAL_RESALLOC_GLOBAL_BUFFER);
      } else {
        result = calResAllocLocal2D(&(s->res[i]), s->device, nx, ny, c, CAL_RESALLOC_GLOBAL_BUFFER);
      }
    }
    break;
  default:
    {
      result = CAL_FALSE;
    }
  }

  if (result != CAL_RESULT_OK) {
    showerror("FATAL alloc. 1");
    exit(-1);
  }

  result = calCtxGetMem(&(s->mem[i]), s->ctx, s->res[i]);
  if (result != CAL_RESULT_OK) {
    showerror("FATAL alloc. 2");
    exit(-1);
  }

  s->obj[i].type = type;
  s->obj[i].nx = nx;
  s->obj[i].ny = ny;
}

void ACCLallocateresource_pinned(int idev, int i, int type, int nx, int ny, void *p, int size)
{
  CALformat c;
  CALresult result = CAL_RESULT_OK;
  int dim;
  ACCL *s = &pp[idev];

  if (calResCreate2D == 0) {
    calExtGetProc((CALextproc*)&calResCreate2D, CAL_EXT_RES_CREATE, "calResCreate2D");
  }
  if (calResCreate1D == 0) {
    calExtGetProc((CALextproc*)&calResCreate1D, CAL_EXT_RES_CREATE, "calResCreate1D");
  }

  c   = (CALformat)(type & 0xff);
  dim = (type >> 10) & 0x1;

  if (dim == 1) {
    result = calResCreate1D(&(s->res[i]), s->device, p, nx, c, size, 0);
  } else {
    result = calResCreate2D(&(s->res[i]), s->device, p, nx, ny, c, size, 0);
  }

  if (result != CAL_RESULT_OK) {
    showerror("FATAL alloc. pinned");
    exit(-1);
  }

  result = calCtxGetMem(&(s->mem[i]), s->ctx, s->res[i]);
  if (result != CAL_RESULT_OK) {
    showerror("FATAL alloc. pinned 22");
    exit(-1);
  }

  s->obj[i].type = type;
  s->obj[i].nx = nx;
  s->obj[i].ny = ny;
}

void ACCLallocateresource(int idev, int i, int type, int nx, int ny)
{
  ACCL *s = &pp[idev];
  int rw;

  ACCLallocateresource0(idev, i, type, nx, ny);

  rw  = (type >> 20) & 0x7;
  if (rw == 1) sprintf((char *)s->obj[i].vn, "i%i", i);
  else if (rw == 2){
    sprintf((char *)s->obj[i].vn, "o%i", s->nout++);
  } else if (rw == 6 || rw == 5){
    sprintf((char *)s->obj[i].vn, "g[]");
  } else {
  }
}

int ACCLbindmemory(int idev, int imod, int i)
{
  ACCL *s = &pp[idev];
  char *vn = (char *)(s->obj[i].vn);
  CALname name;
 
  if (calModuleGetName(&name, s->ctx, s->module[imod], vn) != CAL_RESULT_OK) {
    fprintf(stderr, "no variable %s in the loaded kernel\n", vn);
    showerror("calModuleGetName");
    return -1;
  }

  calCtxSetMem(s->ctx, name, s->mem[i]); 
  return 1;
}

int ACCLrebindmemory(int idev, int imod, int i, const char *vn)
{
  ACCL *s = &pp[idev];
  CALname name;
 
  if (calModuleGetName(&name, s->ctx, s->module[imod], vn) != CAL_RESULT_OK) {
    fprintf(stderr, "no variable %s in the loaded kernel\n", vn);
    showerror("calModuleGetName");
    return -1;
  }

  calCtxSetMem(s->ctx, name, s->mem[i]); 

  strcpy((char *)s->obj[i].vn, vn);
  s->obj[i].mod = imod;
  return 1;
}

int ACCLrebindmemory0(int idev, int imod, int i, const char *vn)
{
  ACCL *s = &pp[idev];
  CALname name;
 
  calModuleGetName(&name, s->ctx, s->module[imod], vn);
  calCtxSetMem(s->ctx, name, s->mem[i]); 

  s->obj[i].mod = imod;
  return 1;
}

int ACCLchangemodbinding(int idev, int imod, int i)
{
  ACCL *s = &pp[idev];
  CALname name;
 
  if (calModuleGetName(&name, s->ctx, s->module[imod], (char *)s->obj[i].vn) != CAL_RESULT_OK) {
    fprintf(stderr, "no variable %s in %i module\n", (char *)s->obj[i].vn, imod);
    showerror("calModuleGetName");
    return -1;
  }
  calCtxSetMem(s->ctx, name, s->mem[i]); 

  s->obj[i].mod = imod;
  return 1;
}

int ACCLallocatememory(int idev, int imod, int i, int type, int nx, int ny)
{
  ACCLallocateresource(idev, i, type, nx, ny);
  pp[idev].obj[i].mod = imod;
  return ACCLbindmemory(idev, imod, i);
}

void ACCLreleasememories(int idev)
{
  ACCL *s = &pp[idev];
  for(int i = 0; i < _n; i++) {
    if (s->res[i] == 0) continue;
    calCtxReleaseMem(s->ctx, s->mem[i]);
    calResFree(s->res[i]);
    s->res[i] = 0;
  }
  s->nout = 0;
}

void ACCLreleasememory(int idev, int i)
{
  ACCL *s = &pp[idev];
  calCtxReleaseMem(s->ctx, s->mem[i]);
  calResFree(s->res[i]);
  s->res[i] = 0;
}

void ACCLdumpresources(int idev)
{
  ACCL *s = &pp[idev];
  for(int i = 0; i < _n; i++) {
    if (s->res[i] == 0) continue;
    fprintf(stderr, "%i %s -> (%i, %i) on %i\n", i, 
	    (char *)s->obj[i].vn, s->obj[i].nx, s->obj[i].ny, s->obj[i].mod);
  }
}

// assuming always nx > pitch 
void *ACCLmapping(int idev, int i)
{
  ACCL *s = &pp[idev];
  void *p;
  CALuint pitch;
  
  if (s->res[i] == 0) return NULL;
  if (calResMap((CALvoid**)&p, &pitch, s->res[i], 0) == CAL_RESULT_ERROR) {
    showerror("calResMap");
    fprintf(stderr, "Resource mapping failed. %i\n", i);    
    return NULL;
  }

  return p;
}

void ACCLunmapping(int idev, int i)
{
  ACCL *s = &pp[idev];
  calResUnmap(s->res[i]);
}

int ACCLwritememory_batch(int idev, int n, int *il, int *sl, void **p_src)
{
  ACCL *s = &pp[idev];
  int k, i;
  for(k = 0; k < n; k++) {
    void *p;
    CALuint pitch;
    i = il[k];
    calResMap((CALvoid**)&p, &pitch, s->res[i], 0);
    memcpy(p, p_src[k], sl[k]);
  }

  for(k = 0; k < n; k++) {
    calResUnmap(s->res[il[k]]);
  }

  return 1;
}

CALresource ACCL_get_res(int idev, int i)
{
  ACCL *s = &pp[idev];
  return s->res[i];
}

int ACCLwritememory(int idev, int i, int size, void *p_src)
{
  void *p_dst = ACCLmapping(idev, i);
  if (p_dst == NULL) return -1;
  memcpy(p_dst, p_src, size);
  ACCLunmapping(idev, i);

  return 1;
}

int ACCLreadmemory(int idev, int i, int size, void *p_dst)
{
  void *p_src = ACCLmapping(idev, i);
  if (p_src == NULL) return -1;
  memcpy(p_dst, p_src, size);
  ACCLunmapping(idev, i);
  return 1;
}

int ACCLwritememory_double(int idev, int i, int n, double *p)
{
  return ACCLwritememory(idev, i, sizeof(double)*n, (void *)p);
}

int ACCLreadmemory_double(int idev, int i, int n, double *p)
{
  return ACCLreadmemory(idev, i, sizeof(double)*n, (void *)p);
}

int ACCLwritememory_float(int idev, int i, int n, float *p)
{
  return ACCLwritememory(idev, i, sizeof(float)*n, (void *)p);
}

int ACCLreadmemory_float(int idev, int i, int n, float *p)
{
  return ACCLreadmemory(idev, i, sizeof(float)*n, (void *)p);
}

int ACCLwritememory_int(int idev, int i, int n, int *p)
{
  return ACCLwritememory(idev, i, sizeof(int)*n, (void *)p);
}

int ACCLreadmemory_int(int idev, int i, int n, int *p)
{
  return ACCLreadmemory(idev, i, sizeof(int)*n, (void *)p);
}

void ACCLsetdomain(int idev, int w, int h)
{
  ACCL *s = &pp[idev];
  s->domain.x = 0;
  s->domain.y = 0;
  s->domain.width  = w;
  s->domain.height = h;
}

void ACCLstart(int idev, int imod)
{
  ACCL *s = &pp[idev];
  calCtxRunProgram(&(s->event), s->ctx, s->main_func[imod], &(s->domain));
}

void ACCLwait(int idev)
{
  ACCL *s = &pp[idev];
  while (calCtxIsEventDone(s->ctx, s->event) == CAL_RESULT_PENDING);
}

void ACCLrun(int idev, int imod)
{
  ACCLstart(idev, imod);
  ACCLwait(idev); 
}

void ACCLrun_on_domain(int idev, int imod, int w, int h)
{
  ACCLsetdomain(idev, w, h);
  ACCLstart(idev, imod);
  ACCLwait(idev); 
}

void ACCLrun_on_domain0(int idev, int imod, int w, int h)
{
  ACCLsetdomain(idev, w, h);
  ACCLstart(idev, imod);
  ACCLwait(idev); 
}

void ACCLrun_on_domain2(int idev, int imod, int x0, int y0, int x1, int y1)
{
  ACCL *s = &pp[idev];
  s->domain.x = x0;
  s->domain.y = y0;
  s->domain.width  = x1;
  s->domain.height = y1;
  ACCLstart(idev, imod);
  ACCLwait(idev); 
}

void ACCLcleanup(int idev)
{
  ACCL *s = &pp[idev];
  ACCLreleasememories(idev);
  for(int i = 0; i < nmodulemax; i++) ACCLunloadkernel(idev, i);

  calCtxDestroy(s->ctx);
  calDeviceClose(s->device);
  calShutdown();
}

void ACCLmemcpy(int idev, int src, int dst) 
{
  ACCL *s = &pp[idev];
  if (s->res[src] == 0 || s->res[dst] == 0) {
    fprintf(stderr, "mem is not allcoated\n");
    exit(-1);
  }

  if (calMemCopy(&(s->event), s->ctx, s->mem[src], s->mem[dst], 0) != CAL_RESULT_OK) {
    showerror("calMemCopy : ");
    return;
  }
  calCtxFlush(s->ctx);

  ACCLwait(idev);
}

void ACCLmemcpy0(int idev, int src, int dst) 
{
  ACCL *s = &pp[idev];
  if (s->res[src] == 0 || s->res[dst] == 0) {
    fprintf(stderr, "mem is not allcoated\n");
    exit(-1);
  }

  if (calMemCopy(&(s->copy_event[src]), s->ctx, s->mem[src], s->mem[dst], 0) != CAL_RESULT_OK) {
    showerror("calMemCopy : ");
    return;
  }
  //  calCtxFlush(s->ctx);
  //  ACCLwait(idev);
}

void ACCLctxflush(int idev)
{
  ACCL *s = &pp[idev];
  calCtxFlush(s->ctx);
}

void ACCLwait0(int idev, int src)
{
  ACCL *s = &pp[idev];
  calCtxFlush(s->ctx);
  while (calCtxIsEventDone(s->ctx, s->copy_event[src]) == CAL_RESULT_PENDING);
}

#ifdef QD
#include <qd/qd_real.h>
#include <qd/dd_real.h>

int ACCLwritememory_dd(int idev, int i, int size, dd_real *mp)
{
  return ACCLwritememory(idev, i, sizeof(dd_real)*size, (void *)mp);
}

int ACCLreadmemory_dd(int idev, int i, int size, dd_real *mp)
{
  return ACCLreadmemory(idev, i, sizeof(dd_real)*size, (void *)mp);
}

int ACCLwritememory_qd(int idev, int i, int size, qd_real *mp)
{
  return ACCLwritememory(idev, i, sizeof(qd_real)*size, (void *)mp);
}

int ACCLreadmemory_qd(int idev, int i, int size, qd_real *mp)
{
  return ACCLreadmemory(idev, i, sizeof(qd_real)*size, (void *)mp);
}
#endif

static PFNCALCTXCREATECOUNTER  calCtxCreateCounterExt;
static PFNCALCTXDESTROYCOUNTER calCtxDestroyCounterExt;
static PFNCALCTXBEGINCOUNTER   calCtxBeginCounterExt;
static PFNCALCTXENDCOUNTER     calCtxEndCounterExt;
static PFNCALCTXGETCOUNTER     calCtxGetCounterExt;
static CALcounter idleCounter;
static CALcounter cacheCounter;
static int perf_flag = 0;

void ACCLsetupcounter(int idev)
{
  ACCL *s = &pp[idev];

  if (calExtSupported((CALextid)CAL_EXT_COUNTERS) != CAL_RESULT_OK)
    exit(-1);

  if (calExtGetProc((CALextproc*)&calCtxCreateCounterExt,
		    (CALextid)CAL_EXT_COUNTERS, "calCtxCreateCounter"))
    exit(-1);
  if (calExtGetProc((CALextproc*)&calCtxDestroyCounterExt,
		    (CALextid)CAL_EXT_COUNTERS, "calCtxDestroyCounter"))
    exit(-1);
  if (calExtGetProc((CALextproc*)&calCtxBeginCounterExt,
		    (CALextid)CAL_EXT_COUNTERS, "calCtxBeginCounter"))
    exit(-1);
  if (calExtGetProc((CALextproc*)&calCtxEndCounterExt,
		    (CALextid)CAL_EXT_COUNTERS, "calCtxEndCounter"))
    exit(-1);
  if (calExtGetProc((CALextproc*)&calCtxGetCounterExt,
		    (CALextid)CAL_EXT_COUNTERS, "calCtxGetCounter"))
    exit(-1);

  if (calCtxCreateCounterExt(&idleCounter, s->ctx, CAL_COUNTER_IDLE) !=
      CAL_RESULT_OK)
    exit(-1);

  if (calCtxCreateCounterExt(&cacheCounter, s->ctx,
			     CAL_COUNTER_INPUT_CACHE_HIT_RATE) != CAL_RESULT_OK)
    exit(-1);

  perf_flag = 1;
}

void ACCL_start_c_idle(int idev)
{
  ACCL *s = &pp[idev];
  calCtxBeginCounterExt(s->ctx, idleCounter);
}

void ACCL_stop_c_idle(int idev)
{
  ACCL *s = &pp[idev];
  calCtxEndCounterExt(s->ctx, idleCounter);
}

void ACCL_start_c_cache(int idev)
{
  ACCL *s = &pp[idev];
  calCtxBeginCounterExt(s->ctx, cacheCounter);
}

void ACCL_stop_c_cache(int idev)
{
  ACCL *s = &pp[idev];
  calCtxEndCounterExt(s->ctx, cacheCounter);
}

float ACCL_c_idle(int idev)
{
  ACCL *s = &pp[idev];

  CALfloat idlePercentage = 0.0f;

  calCtxGetCounterExt(&idlePercentage, s->ctx, idleCounter);

  return idlePercentage * 100.0;
}

float ACCL_c_cache(int idev)
{
  ACCL *s = &pp[idev];
  CALfloat cachePercentage = 0.0f;
  calCtxGetCounterExt(&cachePercentage, s->ctx, cacheCounter);

  return cachePercentage * 100.0;
}
