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
#include <system.h>
#include <board.h>
#include <string.h>

/*...................................................................*/
/* Global Function Declaractions                                     */
/*...................................................................*/

/*...................................................................*/
/*      strchr: return pointer of first occurence of character       */
/*                                                                   */
/*      Inputs: string - the string to search                        */
/*              find - the character to find                         */
/*                                                                   */
/*     Returns: pointer of first occurence or NULL if not found      */
/*...................................................................*/
const char *strchr(const char *string, char find)
{
  int length;

  for (length = 0; ((string[length] != '\0') &&
                    (string[length] != find)); ++length) ;
  if (string[length] != '\0')
    return &string[length];
  return NULL;
}

/*...................................................................*/
/*     strnlen: count the length of a string up to a limit           */
/*                                                                   */
/*      Inputs: string - the string                                  */
/*              length - the maximum string length                   */
/*                                                                   */
/*     Returns: Success (0)                                          */
/*...................................................................*/
int strnlen(const char *string, int length)
{
  int len;

  for (len = 0; string[len]; ++len)
  {
    if (len >= length)
      break;
  }
  return len;
}
/*...................................................................*/
/*    strlen: count the length of a string                           */
/*                                                                   */
/*      Inputs: length - the string length                           */
/*                                                                   */
/*     Returns: Success (0)                                          */
/*...................................................................*/
size_t strlen(const char *string)
{
  int length;

  // Count all bytes until the NULL (0) byte
  for (length = 0; string[length]; ++length) ;

  return length;
}

/*...................................................................*/
/*      strcat: concatonate one string with another                  */
/*                                                                   */
/*      Inputs: dst - the existing string appended to                */
/*              add - the new string to be appended to existing      */
/*                                                                   */
/*     Returns: Success (0)                                          */
/*...................................................................*/
size_t strcat(char *dst, const char *add)
{
  int length = strlen(dst); /*safer : strnlen(dst, 1024);? */

  memcpy(&dst[length], add, strlen(add));
  return length;
}

/*...................................................................*/
/*    strcmp: compare two strings                                    */
/*                                                                   */
/*      Inputs: length - the string length                           */
/*                                                                   */
/*     Returns: Returns zero (0) if same, or positive/negative.      */
/*                                                                   */
/*...................................................................*/
int strcmp(const char *string1, const char *string2)
{
  int length;

  for (length = 0; string1[length] && string2[length]; ++length)
  {
    if (string1[length] != string2[length])
      break;
  }

  if (string1[length] && string2[length])
  {
    if (string1[length] > string2[length])
      return 1;
    else if (string1[length] < string2[length])
      return -1;
  }
  return 0;
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
int memcmp(const void *data1, const void *data2, size_t length)
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
void *memcpy(void *dst, const void *src, size_t length)
{
  int i, bytes;
  u8 *destination = dst;
  const u8 *source = src;

  // Loop until all bytes are copied
  for (bytes = 0; bytes < length; ++bytes)
  {
    // If 64 bits remaining and aligned, copy 64 bytes at a time
    if ((length - bytes > 7) &&
        (((uintptr_t)&destination[bytes] & 7) == 0) &&
        (((uintptr_t)&source[bytes] & 7) == 0))
    {
      u64 *dest64 = (u64 *)&destination[bytes];
      const u64 *src64 = (u64 *)&source[bytes];

      for (i = 0; i < (length - bytes) / 8; ++i)
        dest64[i] = src64[i];

      bytes += i * 8 - 1;
    }

    // If 32 bits remaining and aligned, copy 32 bytes at a time
    if ((length - bytes > 3) &&
        (((uintptr_t)&destination[bytes] & 3) == 0) &&
        (((uintptr_t)&source[bytes] & 3) == 0))
    {
      u32 *dest32 = (u32 *)&destination[bytes];
      const u32 *src32 = (u32 *)&source[bytes];

      for (i = 0; i < (length - bytes) / 4; ++i)
        dest32[i] = src32[i];

      bytes += i * 4 - 1;
    }

    // Otherwise copy one byte
    else
      destination[bytes] = source[bytes];
  }
  return dst;
}

/*...................................................................*/
/*    memset: assign a memory region to a value                      */
/*                                                                   */
/*      Inputs: dst - the destination data region                    */
/*              value - the value of data region                     */
/*              length - the data length to set/write                */
/*                                                                   */
/*     Returns: the dst pointer to the initialized data              */
/*...................................................................*/
void *memset(void *dst, int value, size_t length)
{
  int i;
  u8 *memory = dst;

  // Assign all bytes of the memory to the initial value
  for (i = 0; i < length; ++i)
    memory[i] = value;
  return dst;
}

/*...................................................................*/
/*    memchr: find a value within a memory region                    */
/*                                                                   */
/*      Inputs: addr - the data region to search in                  */
/*              value - the value of data to find                    */
/*              length - the data length to search                   */
/*                                                                   */
/*     Returns: pointer to memory containing this value, or NULL     */
/*...................................................................*/
void *memchr(const void *addr, int value, size_t length)
{
  int i;
  const unsigned char *memory = addr;

  for (i = 0; i < length; ++i)
    if (memory[i] == value)
      break;
  if (i < length)
    return (void *)&memory[i];
  else
    return NULL;
}

