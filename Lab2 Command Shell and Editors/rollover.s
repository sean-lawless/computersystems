/*.....................................................................*/
/*                                                                     */
/*   Module:  loopingadd.s                                             */
/*   Version: 2014.0                                                   */
/*   Purpose: Loop based addition ARM assembly language example        */
/*                                                                     */
/*.....................................................................*/
/*                                                                     */
/*                   Copyright 2015, Sean Lawless                      */
/*                                                                     */
/*                      ALL RIGHTS RESERVED                            */
/*                                                                     */
/* Redistribution and use in source and binary forms, with or without  */
/* modification, are permitted provided that the following conditions  */
/* are met:                                                            */
/*                                                                     */
/*  1. Redistributions in any form, including but not limited to       */
/*     source code or binary, must retain the above copyright notice,  */
/*     this list of conditions and the following disclaimer.           */
/*                                                                     */
/*  2. No additional restrictions may be added to this source code     */
/*     which would place limits on the terms of binary distributions.  */
/*                                                                     */
/* THIS SOFTWARE IS PROVIDED ''AS IS''. ANY EXPRESS OR IMPLIED         */
/* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES   */
/* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE         */
/* DISCLAIMED. IN NO EVENT SHALL ANY AUTHOR AND/OR COPYRIGHT HOLDER BE */
/* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR */
/* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT   */
/* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR  */
/* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          */
/* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE   */
/* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH    */
/* DAMAGE.                                                             */
/*.....................................................................*/
;@ Variables
.section .data
Count: .int 0        ;@ declare Count as Integer of value zero
Count2: .int 0       ;@ declare Count2 as Integer of value zero

.section .text
ldr  r1,=Count       ;@ load R1 register with address of Count
ldr  r2,=Count2      ;@ load R2 register with address of Count2

ldr  r3, [r1]        ;@ load Count1 value into R3 by referencing R1
_loop:
add  r3, r3, #1      ;@ add one to the value
str  r3, [r1]        ;@ store the result to Var1 in RAM
cmp  r3, #0          ;@ compare count to zero
ble  _rollover       ;@ branch to rollover if less or equal
b    _loop           ;@ branch to add one again to Count1
_rollover:
ldr  r3, [r1]        ;@ load Var1 into R3 by referencing R1
add  r3, r3, #1      ;@ add one to the value
str  r3, [r1]        ;@ store the result to Var1 in RAM
b    _loop           ;@ branch to add one again to Count1
