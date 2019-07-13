/*...................................................................*/
/*                                                                   */
/*   Module:  string.h                                               */
/*   Version: 2014.0                                                 */
/*   Purpose: String I/O interface                                   */
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
#ifndef _STRING_H
#define _STRING_H

/*...................................................................*/
/* Global Function Definitions                                       */
/*...................................................................*/
extern void *memcpy(void *dst, const void *src, size_t length);
extern void *memset(void *dst, int value, size_t length);
extern int memcmp(const void *data1, const void *data2, size_t length);
extern void *memchr(const void *addr, int value, size_t length);
extern int isprint(int c);
extern void *quick_memcpy(void *dst, void *src, size_t length);
#define bzero(dst, length) memset(dst, 0, length);

extern const char *strchr(const char *string, char find);
extern int strcmp(const char *string1, const char *string2);
extern size_t strlen(const char *string);
extern int strnlen(const char *string, int length);
extern size_t strcat(char *dst, const char *add);
#define strcpy(dst, src) memcpy(dst, src, strlen(src) + 1)
#define strncpy(dst, src, len) memcpy(dst, src, min(strlen(src)+1, len))
#define strlcpy(dst, src, len) memcpy(dst, src, min(strlen(src)+1, len))
#define memscan(ptr, val, num) memchr(ptr, val, num)

#endif /* _STRING_H */
