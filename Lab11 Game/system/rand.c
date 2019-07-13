/*...................................................................*/
/*                                                                   */
/*   Module:  rand.c                                                 */
/*   Version: 2018.0                                                 */
/*   Purpose: Random number generator                                */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2018, Sean Lawless                    */
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

u32 PrngState;

/*...................................................................*/
/* xor_shift: Perform the Xorshift LFSR agorithm                     */
/*                                                                   */
/*   Input: state the current value or state of the PRNG             */
/*                                                                   */
/*  Return: 32 bit random number                                     */
/*...................................................................*/
u32 xor_shift(u32 state)
{
  // Shift and Xor the state
  // Note all shifts are prime numbers
  // Reference: page 4 of "Xorshift RNGs" by Marsaglia
  state ^= state << 13; // Feed bottom of x back on top of itself
  state ^= state >> 17; // Feed top of x back on bottom of itself
  state ^= state << 5;  // Feed bottom most of x back on top of itself

  // Return the new Xorshift state or value
  return state;
}

/*...................................................................*/
/*   srand: Initialize, or seed, the PRNG                            */
/*                                                                   */
/*   Input: initial system provided initial random value             */
/*...................................................................*/
void srand(u32 initialState)
{
  PrngState = xor_shift(initialState);
}

/*...................................................................*/
/*    rand: Generate a new random value                              */
/*                                                                   */
/*  Return: 32 bit random number                                     */
/*...................................................................*/
u32 rand(void)
{
  PrngState = xor_shift(PrngState);
  return PrngState;
}
