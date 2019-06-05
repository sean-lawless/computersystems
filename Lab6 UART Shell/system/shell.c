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
static int echo(const char *command);
static int led(const char *command);

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
static ShellCmd ShellCommands[] =
{
  {"echo", echo},
  {"ledon", led},
  {"ledoff", led},
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
  UartPuts(command);
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
/*       shell: Run a system shell command                           */
/*                                                                   */
/*       input: command = the entire command                         */
/*...................................................................*/
static int shell(const char *command)
{
  int i, j;

  /* If question '?', print out list of available commands. */
  if (command[0] == '?')
  {
    UartPuts("Available commands are:");
    for (i = 0; ShellCommands[i].command; ++i)
      UartPuts(ShellCommands[i].command);
    return 0;
  }

  /* Search the command table and run any installed commands. */
  for (i = 0; ShellCommands[i].command; ++i)
  {
    /* Loop through length of command, comparing each character */
    /* until either string ends with the NULL character '\0'. */
    for (j = 0; (command[i] != '\0') &&
                (ShellCommands[i].command[j] != '\0') ; ++j)
    {
      /* If character does not match then command does not so break. */
      if (command[j] != ShellCommands[i].command[j])
        break;
    }

    /* If end of shell command reached then all characters match. */
    if (ShellCommands[i].command[j] == '\0')
    {
      /* Execute and return the command result. */
      return ShellCommands[i].function(command);
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
    /* Output the shell prompt '>'. */
    UartPutc('>');

    /* Loop reading user input characters to build 'command' array. */
    /* Break out when 'Enter' key pressed or command length reached. */
    for (i = 0, character = 0; (i < COMMAND_LENGTH) &&
                               (character != '\r'); ++i)
    {
      /* Read a character and put it in the command array. */
      character = UartGetc();
      command[i] = character;

      /* Echo the character back to the user. */
      UartPutc(character);
    }

    /* Output new line and NULL terminate the command string. */
    UartPutc('\n');
    command[i] = 0;

    /* If an empty command entered, loop back to give a new prompt. */
    if ((i == 1) && (character == '\r'))
      continue;

    /* Perform the command. */
    result = shell(command);

    /* Report any errors. */
    if (result > 0)
      UartPuts("command error");
    else if (result == -1)
      UartPuts("command unknown\n");
  }
}

#endif /* ENABLE_SHELL */

