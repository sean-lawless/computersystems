/*...................................................................*/
/*                                                                   */
/*   Module:  string.c                                               */
/*   Version: 2015.0                                                 */
/*   Purpose: String routines                                        */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2015, Sean Lawless                    */
/*                                                                   */
/*                      ALL RIGHTS RESERVED                          */
/*                                                                   */
/* Redistribution and use in source, binary or derived forms, with   */
/* or without modification, are permitted provided that the          */
/* following conditions are met:                                     */
/*                                                                   */
/*  1. Redistributions in any form, including but not limited to     */
/*     source code, binary, or derived works, must include the above */
/*     copyright notice, this list of conditions and the following   */
/*     disclaimer.                                                   */
/*                                                                   */
/*  2. Any change or addition to this copyright notice requires the  */
/*     prior written permission of the above copyright holder.       */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED ''AS IS''. ANY EXPRESS OR IMPLIED       */
/* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES */
/* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE       */
/* DISCLAIMED. IN NO EVENT SHALL ANY AUTHOR AND/OR COPYRIGHT HOLDER  */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/*...................................................................*/
#include <stdio.h>
#include <system.h>

/*...................................................................*/
/* Global Function Declaractions                                     */
/*...................................................................*/

/*...................................................................*/
/*    strlen: count the length of a string                           */
/*                                                                   */
/*      Inputs: length - the string length                           */
/*                                                                   */
/*     Returns: Success (0)                                          */
/*...................................................................*/
int strlen(const char *string)
{
  int length;

  // Count all bytes until the NULL (0) byte
  for (length = 0; string[length]; ++length) ;

  return length;
}

/*...................................................................*/
/*    memcmp: compare two data regions                               */
/*                                                                   */
/*      Inputs: data1 - the first data region                        */
/*              data2 - the second data region                       */
/*              length - the data length to compare                  */
/*                                                                   */
/*     Returns: zero (0) if identical, or positive/negative location.*/
/*...................................................................*/
int memcmp(const void *data1, const void *data2, int length)
{
  int i;

  // Compare each byte and break if not equal.
  for (i = 0; i < length; ++i)
    if (((u8 *)data1)[i] != ((u8 *)data2)[i])
      break;

  // Return zero if equal, or index of unequal byte
  if (i == length)
    return 0;
  else if (((u8 *)data1)[i] > ((u8 *)data2)[i])
    return i + 1; // Positive index if greater than
  else
    return -(i + 1); // Negative index if less than
}

/*...................................................................*/
/*    memcpy: copy data from one region to another                   */
/*                                                                   */
/*      Inputs: dst - the destination data region                    */
/*              src - the source data region                         */
/*              length - the data length to compare                  */
/*                                                                   */
/*     Returns: the dst pointer to the newly copied data             */
/*...................................................................*/
void *memcpy(void *dst, const void *src, int length)
{
  int bytes;
  u8 *destination = dst;
  const u8 *source = src;

  // Loop until all bytes are copied
  for (bytes = 0; bytes < length; ++bytes)
    destination[bytes] = source[bytes];
  return dst;
}
