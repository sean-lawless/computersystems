/*...................................................................*/
/*                                                                   */
/*   Module:  board.c                                                */
/*   Version: 2015.0                                                 */
/*   Purpose: Board support package for Raspberry Pi                 */
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
#include <stdio.h>
#include <string.h>

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define USE_64BIT_HW_CLOCK  TRUE  /* Set FALSE to only use low 32 */
                                  /* bit clock register and software */
                                  /* to manager overflow as in the */
                                  /* companion book. */

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
struct led_state LedState;
u32 LedTime;

extern void XmodemInit(void);
extern void ShellInit(void);

/*...................................................................*/
/* Global Function Definitions                                       */
/*...................................................................*/

/*...................................................................*/
/*  BoardInit: Initialize the RPi board                              */
/*                                                                   */
/*...................................................................*/
void BoardInit(void)
{
  unsigned int select;

  // initialize the LED state
  bzero(&LedState, sizeof(struct led_state));

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

  /* Sleep for one hundred of a second to activate last command. */
  usleep(MICROS_PER_SECOND / 100);

#if RPI == 3
  /* Push GPPUD settings to GPPUDCLK0 GPIO 29. */
  REG32(GPPUDCLK0) = (1 << 29); /* GPIO 29 */
#else
  /* Push GPPUD settings to GPPUDCLK1 GPIO 47. */
  REG32(GPPUDCLK1) = (1 << (47 - 32)); /* GPIO 47 */
#endif

  /* Sleep for one hundred of a second to activate last command. */
  usleep(MICROS_PER_SECOND / 100);
#endif /* RPI == 4 */


#if ENABLE_JTAG
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
  usleep(MICROS_PER_MILLISECOND); /* 1ms hold time */

  // Apply to all the JTAG GPIO pins
  REG32(GPPUDCLK0) = (1 << 22) | (1 << 23) | (1 << 24) | (1 << 25) |
                     (1 << 26) | (1 << 27);
  usleep(MICROS_PER_MILLISECOND); /* 1ms hold time */
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
#endif

#if ENABLE_UART0

  /* Initialize the primary UART. */
  Uart0Init();

#if ENABLE_SHELL
  bzero(&Uart0State, sizeof(struct shell_state));

  /* initialize a shell task for the primary UART */
  Uart0State.result = TASK_FINISHED;
  Uart0State.cmd = NULL;
  Uart0State.getc = Uart0Getc;
  Uart0State.putc = Uart0Putc;
  Uart0State.puts = Uart0Puts;
  Uart0State.check = Uart0RxCheck;
  Uart0State.flush = Uart0Flush;

  /* display the introductory splash */
  Uart0State.puts("Computer Systems");
  Uart0State.puts("  Using priority loop scheduler");
  Uart0State.puts("Copyright 2015-2019 Sean Lawless.");
  Uart0State.puts("  All rights reserved.\n");
  Uart0State.puts("Connected to primary UART interface.");
  Uart0State.puts("'?' for a list of commands");
#else
  /* display the introductory splash */
  Uart0Puts("Computer Systems");
  Uart0Puts("\tCopyright 2015-2019 Sean Lawless.");
  Uart0State.puts("  All rights reserved.\n");
  Uart0Puts("Connected to primary UART interface.");
  Uart0Puts("'?' for a list of commands");
#endif
#endif /* ENABLE_UART0 */

#if ENABLE_UART1
  /* Initialize the secondary UART. */
  Uart1Init();

#if ENABLE_SHELL
  bzero(&Uart1State, sizeof(struct shell_state));

  /* initialize a shell task for the secondary UART */
  Uart1State.result = TASK_FINISHED;
  Uart1State.cmd = NULL;
  Uart1State.getc = Uart1Getc;
  Uart1State.putc = Uart1Putc;
  Uart1State.puts = Uart1Puts;
  Uart1State.check = Uart1RxCheck;
  Uart1State.flush = Uart1Flush;

  /* display the introductory splash */
  Uart1State.puts("Computer Systems");
  Uart1State.puts("  Copyright 2015-2019 Sean Lawless");
  Uart1State.puts("All rights reserved\n");
  Uart1State.puts("Connected to secondary UART interface.");
  Uart1State.puts("'?' for a list of commands");
#else
  /* display the introductory splash */
  Uart1Puts("Computer Systems");
  Uart1Puts("  Copyright 2015-2019 Sean Lawless\n");
  Uart1Puts.puts("All rights reserved\n");
  Uart1Puts("Connected to secondary UART interface.");
  Uart1Puts("'?' for a list of commands");
#endif
#endif /* ENABLE_UART1 */

#if ENABLE_XMODEM
  XmodemInit();
#endif

#if ENABLE_SHELL
  ShellInit();
#endif

  /* Initialize LED task blinker. */
  LedTime = MICROS_PER_SECOND;
  LedState.expire = TimerRegister(LedTime);
  LedState.state = 0;
}

/*...................................................................*/
/*      LedOn: Turn on the activity LED                              */
/*                                                                   */
/*...................................................................*/
void LedOn(void)
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
}

/*...................................................................*/
/*     LedOff: Turn off the activity LED                             */
/*                                                                   */
/*...................................................................*/
void LedOff(void)
{
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
}

/*..................................................................*/
/* LedPoll: poll the led timer and toggle LED if expired            */
/*                                                                  */
/* returns: exit error                                              */
/*..................................................................*/
int LedPoll(void *data)
{
  struct led_state *state = data;

  /* check if the timer has expired */
  if (TimerRemaining(&state->expire) == 0)
  {
    /* if on then turn off */
    if (state->state)
    {
      LedOff();
      state->state = 0;
    }

    /* otherwise turn on */
    else
    {
      LedOn();
      state->state = 1;
    }
    state->expire = TimerRegister(LedTime);
  }

  return TASK_IDLE;
}

#if USE_64BIT_HW_CLOCK
/*...................................................................*/
/* TimerRegister: Register an expiration time                        */
/*                                                                   */
/*      Input: microseconds until timer expires                      */
/*                                                                   */
/*    Returns: resulting expiration time                             */
/*...................................................................*/
struct timer TimerRegister(u64 microseconds)
{
  struct timer tw;
  u64 now;

  /* Retrieve the current time from the 1MHZ hardware clock. */
  now = REG32(TIMER_CLO);  /* low order time */
  now |= ((u64)REG32(TIMER_CHI) << 32); /* high order time */

  /* Calculate and return the expiration time of the new timer. */
  tw.expire = now + microseconds;

  /* Return the created timer. */
  return tw;
}

/*...................................................................*/
/* TimerRemaining: Check if a registered timer has expired           */
/*                                                                   */
/*      Input: expire - clock time of expiration in microseconds     */
/*             unused - unused on RPi as it has 64 bit precision     */
/*                                                                   */
/*    Returns: Zero (0) or microseconds until timer expiration       */
/*...................................................................*/
u64 TimerRemaining(struct timer *tw)
{
  u64 now;

  /* Retrieve the current time from the 1MHZ hardware clock. */
  now = REG32(TIMER_CLO); /* low order time */
  now |= ((u64)REG32(TIMER_CHI) << 32); /* high order time */

  /* Return zero if timer expired. */
  if (now > tw->expire)
    return 0;

  /* Return time until expiration if not expired. */
  return tw->expire - now;
}

/*...................................................................*/
/*   TimerNow: Return the current time in clock ticks                */
/*                                                                   */
/*...................................................................*/
u64 TimerNow(void)
{
  u64 now;

  /* Retrieve the current time from the 1MHZ hardware clock. */
  now = REG32(TIMER_CLO); /* low order time */
  now |= ((u64)REG32(TIMER_CHI) << 32); /* high order time */

  /*
  ** Return the current time.
  */
  return now;
}

#else
/*...................................................................*/
/* TimerRegister: Register an expiration time                        */
/*                                                                   */
/*      Input: microseconds until timer expires                      */
/*                                                                   */
/*    Returns: resulting timer structure                             */
/*...................................................................*/
struct timer TimerRegister(u64 microseconds)
{
  struct timer tw;
  u64 now;

  /* Retrieve the current time ticks of T1_CLOCK_SECOND frequency. */
  now = REG32(TIMER_CLO);  /* 32 bits of time */

  /* Scale hardware time to microseconds by multiplying by the */
  /* magnitude (scale) of the difference in time frequency. */
  if (MICROS_PER_SECOND > T1_CLOCK_SECOND)
    now *= (MICROS_PER_SECOND - T1_CLOCK_SECOND);

  /* Calculate and return the expiration time of the new timer. */
  tw.expire = now + microseconds;

  /* Return the created timer. */
  return tw;
}

/*...................................................................*/
/* TimerRemaining: Check if a registered timer has expired           */
/*                                                                   */
/*     Inputs: expire - clock time of expiration in microseconds     */
/*               last - the last time this function was called       */
/*                                                                   */
/*    Returns: Zero (0) or current time if unexpired                 */
/*...................................................................*/
u64 TimerRemaining(struct timer *tw)
{
  u64 now;

  /* Retrieve the current time from the 1MHZ hardware clock. */
  now = REG32(TIMER_CLO); /* 32 bits of time */

  /* Scale hardware time to microseconds by multiplying by the */
  /* magnitude (scale) of the difference in time frequency. */
  if (MICROS_PER_SECOND > T1_CLOCK_SECOND)
    now *= (MICROS_PER_SECOND - T1_CLOCK_SECOND);

  /* If low order 32 bits of 'last' > 'now' a rollover occurred. */
  if ((tw->last & 0xFFFFFFFF) > now)
    now += ((u64)1 << 32);

  /* Add saved high order bits from the last. */
  now += tw->last & 0xFFFFFFFF00000000;

  /* Return zero if timer expired. */
  if (now > tw->expire)
    return 0;

  /* Return time until expiration if not expired. */
  tw->last = now;
  return tw->expire - now;
}

/*.....................................................................*/
/*   TimerNow: Return the current time in clock ticks                  */
/*                                                                     */
/*.....................................................................*/
u64 TimerNow(void)
{
  u64 now;

  /* Retrieve the current time from the 1MHZ hardware clock. */
  now = REG32(TIMER_CLO); /* low order time */

  /* Scale hardware time to microseconds by multiplying by the */
  /* magnitude (scale) of the difference in time frequency. */
  if (MICROS_PER_SECOND > T1_CLOCK_SECOND)
    now *= (MICROS_PER_SECOND - T1_CLOCK_SECOND);

  /*
  ** Return the current time.
  */
  return now;
}

#endif

/*...................................................................*/
/*     usleep: Wait or sleep for an amount of microseconds           */
/*                                                                   */
/*      Input: microseconds to sleep                                 */
/*...................................................................*/
void usleep(u64 microseconds)
{
  struct timer tw;

  /* Create a timer that expires 'microseconds' from now. */
  tw = TimerRegister(microseconds);

  /* Loop checking the timer until it expires. */
  for (;TimerRemaining(&tw) > 0;)
    ; // Do nothing
}
