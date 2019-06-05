/*...................................................................*/
/*                                                                   */
/*   Module:  string.c                                               */
/*   Version: 2015.0                                                 */
/*   Purpose: String routines for implementing <string.h>            */
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

  for (i = 0; i < length; ++i)
  {
    if (((unsigned char *)data1)[i] != ((unsigned char *)data2)[i])
      break;
  }

  if (i == length)
    return 0;
  else if (((unsigned char *)data1)[i] > ((unsigned char *)data1)[i])
    return i + 1;
  else
    return -(i + 1);
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
  int i;
  unsigned char *destination = dst;
  const unsigned char *source = src;
  unsigned int *dest32 = dst;
  const unsigned int *src32 = src;

  if ((((unsigned int)dst & 3) == 0) && (((unsigned int)src & 3) == 0))
  {
    for (i = 0; i < length / 4; ++i)
      dest32[i] = src32[i];
    for (i = 0; i < length % 4; ++i)
      destination[i] = source[i];
  }
  else
  {
    for (i = 0; i < length; ++i)
      destination[i] = source[i];
  }
  return dst;
}
