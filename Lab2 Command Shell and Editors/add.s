/*.....................................................................*/
/*                                                                     */
/*   Module:  add.s                                                    */
/*   Version: 2015.0                                                   */
/*   Purpose: Addition ARM assembly language example                   */
/*                                                                     */
/*.....................................................................*/
/*                                                                     */
/*                   Copyright 2014, Sean Lawless                      */
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
Var1: .int 0         ;@ declare Var1 as an integer with value zero

.section .text
ldr  r1,=Var1        ;@ load R1 register with address in RAM of Var1
ldr  r4, [r1]        ;@ load value of Var1 by referencing R1 address
add  r4, r4, #1      ;@ add one to the value
str  r4, [r1]        ;@ store the result to Var1 in RAM
