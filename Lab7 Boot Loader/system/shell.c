/*...................................................................*/
/*                                                                   */
/*   Module:  shell.c                                                */
/*   Version: 2015.0                                                 */
/*   Purpose: system shell or command line interface                 */
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
#include <board.h>

#if ENABLE_SHELL

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define COMMAND_LENGTH   80

/*...................................................................*/
/* Type definitions                                                  */
/*...................................................................*/
typedef struct
{
  char *command;
  int (*function)(const char *command);
} ShellCmd;

/*
** Shell Functions
*/
#if ENABLE_BOOTLOADER
static int xmodem(const char *command);
#endif
static int echo(const char *command);
static int led(const char *command);
static int run(const char *command);
static int quit(const char *command);

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
static ShellCmd ShellCommands[] =
{
  {"echo", echo},
  {"ledon", led},
  {"ledoff", led},
#if ENABLE_BOOTLOADER
  {"xmodem", xmodem},
#endif
  {"run", run},
  {"exit", quit},
  {0, 0},
};

/*...................................................................*/
/* Local function definitions                                        */
/*...................................................................*/

/*...................................................................*/
/*    echo: Echo a string to all consoles                            */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: 0                                                        */
/*.................................................................. */
static int echo(const char *command)
{
  /* Echo the command to the console. */
  puts(command);
  return 0;
}

/*...................................................................*/
/*     led: Command to turn the LED on or off                        */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: 0                                                        */
/*...................................................................*/
static int led(const char *command)
{
  /* Check the last letter of the command */

  /* If ‘ledon’ then last array element 4 is ‘n’ */
  if (command[4] == 'n')
    LedOn();

  /* Otherwise if ‘ledoff’ then array element 4 is ‘f’ */
  else if (command[4] == 'f')
    LedOff();
  else
    UartPuts("error – shell mishandled command?");

  return 0;
}

/*...................................................................*/
/*     run: Exectute application                                     */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: 0                                                        */
/*...................................................................*/
static int run(const char *command)
{
  /* Branch to the application. */
  _branch_to_run();
  return 0; /* not reached */
}

/*...................................................................*/
/*    quit: Perform the exit command                                 */
/*                                                                   */
/*   Input: command = the entire command                             */
/*                                                                   */
/*  return: 0                                                        */
/*...................................................................*/
static int quit(const char *command)
{
  /* Branch to the bootloader. */
  _branch_to_boot();
  return 0; /* not reached */
}

#if ENABLE_BOOTLOADER
/*...................................................................*/
/*  xmodem: Perform the xmodem download and run command              */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: 0                                                        */
/*...................................................................*/
static int xmodem(const char *command)
{
  int length;
  unsigned char *base = (unsigned char *)_run_location();
  u32 size = _run_size();

  /* Wait for an Xmodem download. */
   puts("Waiting for Xmodem to receive target image...");
  length = XmodemDownload(base, size);

  /* If data was downloaded report the status. */
  if (length > 0)
  {
    usleep(MICROS_PER_SECOND / 100); /* 1/100 second delay */
    if (length == size)
    {
      puts("Download failed, length exceeded maximum");
      puts("Increase the _run_size()");
    }
    else
      puts("Download complete, type 'run' to execute the application");
  }

  /* Otherwise report the failure. */
  else
  {
    if (length < 0)
      puts("Download failed");
    else
      puts("Download timed out");
  }

  /* Return command completion. */
  return 0;
}
#endif

/*...................................................................*/
/*   shell: Run a system shell command                               */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: Result of executed command, or -1 if command not found   */
/*...................................................................*/
static int shell(const char *command)
{
  int i;

  /* If question '?', print out list of available commands. */
  if (command[0] == '?')
  {
    puts("Available commands are:");
    for (i = 0; ShellCommands[i].command; ++i)
      puts(ShellCommands[i].command);
    puts("");
    return 0;
  }

  /* Search the command table and run any installed commands. */
  for (i = 0; ShellCommands[i].command; ++i)
  {
    /* On match, execute and return command with parameters.*/
    if (memcmp(command, ShellCommands[i].command,
               strlen(ShellCommands[i].command)) == 0)
    {
      u32 result;
      u64 command_time = TimerNow();

      /* Perform the shell command. */
      result = ShellCommands[i].function(command);

      /* Calculate clock delta and return command result. */
      command_time = TimerNow() - command_time;
      putu32(command_time);
      return result;
    }
  }

  /* Return command not found. */
  return -1;
}

/*...................................................................*/
/* Global function definitions                                       */
/*...................................................................*/

/*...................................................................*/
/* SystemShell: system shell executes commands until 'quit'          */
/*                                                                   */
/*...................................................................*/
void SystemShell(void)
{
  int result = 0, i;
  char character, command[COMMAND_LENGTH];

  /* Loop to read and perform the shell commands from the UART. */
  for (;;)
  {
    /* Output a different shell prompt for bootloader and console. */
#if ENABLE_BOOT
    putchar('b');
    putchar('o');
    putchar('o');
    putchar('t');
#else
    putchar('s');
    putchar('h');
    putchar('e');
    putchar('l');
    putchar('l');
#endif
    putchar('>');

    /* Loop reading user input characters to build 'command' array. */
    /* Break out when 'Enter' key pressed or command length reached. */
    for (i = 0, character = 0; (i < COMMAND_LENGTH) &&
                               (character != '\r'); ++i)
    {
      /* Read a character and put it in the command array. */
      character = getchar();
      command[i] = character;

      /* Echo the character back to the user. */
      putchar(character);
    }

    /* Output new line and NULL terminate the command string. */
    putchar('\n');
    command[i] = 0;

    /* If an empty command entered, loop back to give a new prompt. */
    if ((i == 1) && (character == '\r'))
      continue;

    /* Perform the command. */
    result = shell(command);

    /* Report any errors. */
    if (result > 0)
      puts("command failed");
    else if (result == -1)
      puts("command unknown\n");
  }
}

#endif /* ENABLE_SHELL */

