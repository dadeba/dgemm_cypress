mdef(0)_out(1)_in(3)
; double2 dmad
dmad out0.xy, in0, in1.xy, in2.xy
dmad out0.zw, in0, in1.zw, in2.zw
mend
mdef(1)_out(1)_in(4)
; double2 dmad
dmad out0.xy, in0, in1, in3.xy
dmad out0.zw, in0, in2, in3.zw
mend
il_ps_2_0
;
; Copyright (c) 2010- 
;      NAKASATO, Naohito
;      All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
; SUCH DAMAGE.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 4x4 outer product version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
dcl_input_interp(linear) v0.xy
dcl_resource_id(0)_type(2d,unnorm)_fmtx(unknown)_fmty(unknown)_fmtz(unknown)_fmtw(unknown) ;A
dcl_resource_id(1)_type(2d,unnorm)_fmtx(unknown)_fmty(unknown)_fmtz(unknown)_fmtw(unknown) ;B
dcl_literal l0, 0.0, 1.0, 2.0, 3.0
dcl_literal l1, 4.0, 1.0, 2.0, 3.0
dcl_literal l2, -1.0, 1.0, 2.0, 3.0
dcl_cb cb0[1]
dcl_cb cb1[1]
dcl_cb cb2[1]

flr r100.xy, v0.xy
mov r888, r100.xy
mul r100.x, r100.x, l1.x ; x4
mul r100.y, r100.y, l0.z ; x2

mov r100.z, l2.x
mul r100.z, r100.z, l0.z ; -2
mov r100.w, l2.x         ; -1

mov r50, l0.xxxx
mov r51, l0.xxxx
mov r52, l0.xxxx
mov r53, l0.xxxx
mov r54, l0.xxxx
mov r55, l0.xxxx
mov r56, l0.xxxx
mov r57, l0.xxxx

whileloop
  add r100.z, r100.z, l0.z ; +2
  add r100.w, r100.w, l0.y ; +1
  eq r999, r100.z, cb0[0].y
  break_logicalnz r999.x

  ; read A
  sample_resource(0)_sampler(0)_aoffimmi(0.0,0.0,0.0) r60, r100.yz
  sample_resource(0)_sampler(1)_aoffimmi(1.0,0.0,0.0) r61, r100.yz

  ; read B
  sample_resource(1)_sampler(4)_aoffimmi(0.0,0.0,0.0) r30, r100.wx
  sample_resource(1)_sampler(5)_aoffimmi(0.0,1.0,0.0) r31, r100.wx
  sample_resource(1)_sampler(6)_aoffimmi(0.0,2.0,0.0) r32, r100.wx
  sample_resource(1)_sampler(7)_aoffimmi(0.0,3.0,0.0) r33, r100.wx

  mcall(1)(r50),(r60.xy,r30.xy,r31.xy,r50);
  mcall(1)(r51),(r60.xy,r32.xy,r33.xy,r51);

  mcall(1)(r52),(r60.zw,r30.xy,r31.xy,r52);
  mcall(1)(r53),(r60.zw,r32.xy,r33.xy,r53);

  mcall(1)(r54),(r61.xy,r30.xy,r31.xy,r54);
  mcall(1)(r55),(r61.xy,r32.xy,r33.xy,r55);

  mcall(1)(r56),(r61.zw,r30.xy,r31.xy,r56);
  mcall(1)(r57),(r61.zw,r32.xy,r33.xy,r57);

  ; read A
  sample_resource(0)_sampler(0)_aoffimmi(0.0,1.0,0.0) r60, r100.yz
  sample_resource(0)_sampler(1)_aoffimmi(1.0,1.0,0.0) r61, r100.yz

  mcall(1)(r50),(r60.xy,r30.zw,r31.zw,r50);
  mcall(1)(r51),(r60.xy,r32.zw,r33.zw,r51);

  mcall(1)(r52),(r60.zw,r30.zw,r31.zw,r52);
  mcall(1)(r53),(r60.zw,r32.zw,r33.zw,r53);

  mcall(1)(r54),(r61.xy,r30.zw,r31.zw,r54);
  mcall(1)(r55),(r61.xy,r32.zw,r33.zw,r55);

  mcall(1)(r56),(r61.zw,r30.zw,r31.zw,r56);
  mcall(1)(r57),(r61.zw,r32.zw,r33.zw,r57);
endloop

; compute linear adress
mov r100.xy, r888.xy
mul r100.x, r100.x, l0.z ; x2
mul r100.y, r100.y, l1.x ; x4

mov r1, r100.xy
mul r2.x, r1.y, cb0[0].x
add r2.x, r2.x, r1.x
ftoi r2.x, r2.x

iadd r10, r2.xxxx, cb1[0] 

mov r60, g[r10.x+0]
mov r61, g[r10.x+1]
mov r62, g[r10.y+0]
mov r63, g[r10.y+1]
mov r64, g[r10.z+0]
mov r65, g[r10.z+1]
mov r66, g[r10.w+0]
mov r67, g[r10.w+1]

dmul r60.xy, r60.xy, cb2[0].xy
dmul r60.zw, r60.zw, cb2[0].xy
dmul r61.xy, r61.xy, cb2[0].xy
dmul r61.zw, r61.zw, cb2[0].xy
dmul r62.xy, r62.xy, cb2[0].xy
dmul r62.zw, r62.zw, cb2[0].xy
dmul r63.xy, r63.xy, cb2[0].xy
dmul r63.zw, r63.zw, cb2[0].xy
dmul r64.xy, r64.xy, cb2[0].xy
dmul r64.zw, r64.zw, cb2[0].xy
dmul r65.xy, r65.xy, cb2[0].xy
dmul r65.zw, r65.zw, cb2[0].xy
dmul r66.xy, r66.xy, cb2[0].xy
dmul r66.zw, r66.zw, cb2[0].xy
dmul r67.xy, r67.xy, cb2[0].xy
dmul r67.zw, r67.zw, cb2[0].xy

mcall(0)(r50),(cb2[0].zw,r50,r60)
mcall(0)(r51),(cb2[0].zw,r51,r61)
mcall(0)(r52),(cb2[0].zw,r52,r62)
mcall(0)(r53),(cb2[0].zw,r53,r63)
mcall(0)(r54),(cb2[0].zw,r54,r64)
mcall(0)(r55),(cb2[0].zw,r55,r65)
mcall(0)(r56),(cb2[0].zw,r56,r66)
mcall(0)(r57),(cb2[0].zw,r57,r67)

mov g[r10.x+0], r50
mov g[r10.x+1], r51
mov g[r10.y+0], r52
mov g[r10.y+1], r53
mov g[r10.z+0], r54
mov g[r10.z+1], r55
mov g[r10.w+0], r56
mov g[r10.w+1], r57

ret_dyn
endmain

end
