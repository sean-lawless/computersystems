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
Count:               ;@ declare Count as an Integer with value zero
.int 0

.section .text
ldr  r1,=Count       ;@ load R1 register with address in RAM of Count
ldr  r3, [r1]        ;@ load value of Var1 by referencing R1 address

_save:
str  r3, [r1]        ;@ store the result to Var1 in RAM
mov  r2, #1          ;@ initialize the save count to one

_loop:
add  r3, r3, #1      ;@ add one to the value
add  r2, r2, #1      ;@ add one to the save count
cmp  r2, #10         ;@ compare the store count
bgt  _save
b    _loop
