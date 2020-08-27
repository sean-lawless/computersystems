/*...................................................................*/
/*                                                                   */
/*   Module:  main.c                                                 */
/*   Version: 2015.0                                                 */
/*   Purpose: Raspberry Pi LED test                                  */
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

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
/*
 * Configure number of loops reading HW register to wait one second
*/
#define TOGGLE_LOOP_CNT  5000000 /* 5MHz is about 1 second on B+ */
                                 /* and -O2 compiler optimizations. */

/*...................................................................*/
/* Global Function Definitions                                       */
/*...................................................................*/

// Precursor of the _start assembly function
//   Assign default stack pointer (SP)
asm("mov sp,#0x8000");

/*...................................................................*/
/*        main: Application Entry Point                              */
/*                                                                   */
/*     Returns: Exit error                                           */
/*...................................................................*/
int main(void)
{
  unsigned int i, select;

//start

  /* Enable LED. 3 bits per GPIO so 10 GPIOs per select register. */
#if RPI == 4
  /* GPIO 42 is 2nd register in GPFSEL4, so 2 * 3 bits or bit 6. */
  /* Clear the 3 bit range (7) starting at bit 6 */
  select = REG32(GPFSEL4);
  select &= ~(7 << 6);
#elif RPI == 3
  /* GPIO 29 is select register 2, number 9. 9 * 3 bits is bit 27 */
  /* Clear the 3 bit range (7) starting at bit 27 */
  select = REG32(GPFSEL2);
  select &= ~(7 << 27);
#elif RPI == 0
  /* GPIO 16 is select register 1, number 6. 6 * 3 bits is bit 18 */
  /* Clear the 3 bit range (7) starting at bit 18 */
  select = REG32(GPFSEL1);
  select &= ~(7 << 18);
#else
  /* GPIO 47 is 7th register in GPFSEL4, so 7 * 3 bits or bit 21. */
  /* Clear the 3 bit range (7) starting at bit 21 */
  select = REG32(GPFSEL4);
  select &= ~(7 << 21);
#endif

  /* 3 bits per GPIO, input (0), output (1) and alternate select */
  /* 0 through 5. */

#if RPI == 4
  /* Configure the LED (GPIO 42) starting at bit 6, as output (1). */
  select |= (GPIO_OUTPUT << 6);
  REG32(GPFSEL4) = select;
#elif RPI == 3
  /* Configure LED (GPIO 29) starting at bit 27, as output (1). */
  select |= (GPIO_OUTPUT << 27);
  REG32(GPFSEL2) = select;
#elif RPI == 0
  /* Configure LED (GPIO 16) starting at bit 18, as output (1). */
  select |= (GPIO_OUTPUT << 18);
  REG32(GPFSEL1) = select;
#else
  /* Configure the LED (GPIO 47) starting at bit 21, as output (1). */
  select |= (GPIO_OUTPUT << 21);
  REG32(GPFSEL4) = select;
#endif

#if RPI == 4
  /* GPIO 42, 2 bits per PUP GPIO */
  select = REG32(GPIO_PUP_PDN_CNTRL_REG2);
  select &= ~(3 << ((42 - 32) * 2)); // clear old value
  REG32(GPIO_PUP_PDN_CNTRL_REG2) = select | (GPPUP_PULL_UP) << ((42 - 32) * 2);
#else
  /* GPPUD - GPio Pin Up Down configuration */
  /*   (0) disable pull up and pull down to float the GPIO */
  /*   (1 << 0) enable pull down (low) */
  /*   (1 << 1) enable pull up (high) */

  /* Always pull up (high) for LEDs as they require voltage. */
  REG32(GPPUD) = GPPUD_PULL_UP;

  /* Loop to wait until GPPUD assignment persists. */
  for (i = 0; i < TOGGLE_LOOP_CNT / 1000; ++i)
    select = REG32(GPFSEL4);

#if RPI == 3
  /* Push GPPUD settings to GPPUDCLK0 GPIO 29. */
  REG32(GPPUDCLK0) = (1 << 29); /* GPIO 29 */
#else
  /* Push GPPUD settings to GPPUDCLK1 GPIO 47. */
  REG32(GPPUDCLK1) = (1 << (47 - 32)); /* GPIO 47 */
#endif

  /* Loop to wait until GPPUD assignment persists. */
  for (i = 0; i < TOGGLE_LOOP_CNT / 1000; ++i)
    select = REG32(GPFSEL4);
#endif /* RPI == 4 */


#if RPI == 4
  /* 2 bits per PUP GPIO */
  select = REG32(GPIO_PUP_PDN_CNTRL_REG1);
  select &= ~(3 << ((22 - 16) * 2)); // Clear old GPIO 22 value
  select &= ~(3 << ((23 - 16) * 2)); // Clear old GPIO 23 value
  select &= ~(3 << ((24 - 16) * 2)); // Clear old GPIO 24 value
  select &= ~(3 << ((25 - 16) * 2)); // Clear old GPIO 25 value
  select &= ~(3 << ((26 - 16) * 2)); // Clear old GPIO 26 value
  select &= ~(3 << ((27 - 16) * 2)); // Clear old GPIO 27 value
  REG32(GPIO_PUP_PDN_CNTRL_REG1) = select; /* commit changes */
#else
  /* Disable pull up/down for the next configured GPIO. */
  REG32(GPPUD) = GPPUD_OFF;

  /* Loop to wait until GPPUD assignment persists. */
  for (i = 0; i < TOGGLE_LOOP_CNT / 1000; ++i)
    select = REG32(GPFSEL4);

  // Apply to all the JTAG GPIO pins
  REG32(GPPUDCLK0) = (1 << 22) | (1 << 23) | (1 << 24) | (1 << 25) |
                     (1 << 26) | (1 << 27);

  /* Loop to wait until GPPUD assignment persists. */
  for (i = 0; i < TOGGLE_LOOP_CNT / 1000; ++i)
    select = REG32(GPFSEL4);
#endif

  // Select level alternate 4 to enable JTAG
  select = REG32(GPFSEL2);
  select &= ~(7 << 6); //gpio22
  select |= GPIO_ALT4 << 6; //alt4 ARM_TRST
  select &= ~(7 << 9); //gpio23
  select |= GPIO_ALT4 << 9; //alt4 ARM_RTCK
  select &= ~(7 << 12); //gpio24
  select |= GPIO_ALT4 << 12; //alt4 ARM_TDO
  select &= ~(7 << 15); //gpio25
  select |= GPIO_ALT4 << 15; //alt4 ARM_TCK
  select &= ~(7 << 18); //gpio26
  select |= GPIO_ALT4 << 18; //alt4 ARM_TDI
  select &= ~(7 << 21); //gpio27
  select |= GPIO_ALT4 << 21; //alt4 ARM_TMS
  REG32(GPFSEL2) = select;

  /* Loop turning the activity LED on and off. */
  for (;;)
  {
    /* Turn on the activity LED. */
#if RPI == 4
    /* RPI 4 has LED at GPIO 42, so set GPIO 42. */
    REG32(GPSET1) = 1 << (42 - 32);
#elif RPI == 3
    /* RPI 3 has LED at GPIO 29, so set GPIO 29. */
    REG32(GPSET0) = 1 << 29;
#elif RPI == 0
    /* RPI 1A and B have LED at GPIO 16, so set GPIO 16. */
    REG32(GPSET0) = 1 << 16;
#else
    /* Other RPIs have LED at GPIO 47, so set GPIO 47. */
    REG32(GPSET1) = 1 << (47 - 32);
#endif

    // Loop to wait a bit
    for (i = 0; i < TOGGLE_LOOP_CNT; ++i) /* loop to pause LED on */
      select = REG32(GPFSEL4);

    /* Turn off the activity LED. */
#if RPI == 4
    /* RPI 4 has LED at GPIO 42, so clear GPIO 42. */
    REG32(GPCLR1) = 1 << (42 - 32);
#elif RPI == 3
    /* RPI 3 has LED at GPIO 29, so clear GPIO 29. */
    REG32(GPCLR0) = 1 << 29;
#elif RPI == 0
    /* RPI 1A and B have LED at GPIO 16, so clear GPIO 16. */
    REG32(GPCLR0) = 1 << 16;
#else
    /* Other RPIs have LED at GPIO 47, so clear GPIO 47. */
    REG32(GPCLR1) = 1 << (47 - 32);
#endif

    // Loop to wait a bit
    for (i = 0; i < TOGGLE_LOOP_CNT; ++i) /* loop to pause LED off */
      select = REG32(GPFSEL4);
  }
  return select;
}
