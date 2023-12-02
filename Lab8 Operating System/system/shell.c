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
#include <string.h>
#include <system.h>
#include <board.h>

#if ENABLE_SHELL

#define MAX_SHELL_COMMANDS     16

/*
** Shell Functions
*/
#if ENABLE_XMODEM
static int xmodem(const char *command);
#endif
static int echo(const char *command);
static int run(const char *command);
static int quit(const char *command);

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
static struct ShellCmd ShellCommands[MAX_SHELL_COMMANDS];
struct shell_state Uart0State, Uart1State;

/*...................................................................*/
/* Local function definitions                                        */
/*...................................................................*/

/*...................................................................*/
/*    echo: Echo a string to all consoles                            */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_FINISHED                                            */
/*...................................................................*/
static int echo(const char *command)
{
  /* Echo the command to all consoles. */
  puts(command);
  return TASK_FINISHED;
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
  return TASK_FINISHED; /* not reached */
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
  return TASK_FINISHED; /* not reached */
}

#if ENABLE_XMODEM
/*...................................................................*/
/*  xmodem: Perform the xmodem download and run command              */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_IDLE if in progress, TASK_FINISHED if complete      */
/*...................................................................*/
void *XmodemData;
static int xmodem(const char *command)
{
  int status;

  if (XmodemData == NULL)
  {
    puts("Waiting for Xmodem to receive target image...");
    XmodemData = XmodemStart((u8 *)_run_location(), _run_size());
    if (XmodemData == NULL)
    {
      puts("Xmodem already in progress, please try again later");
      return TASK_FINISHED;
    }
  }

  /* Periodic execution of the xmodem transfer. */
  status = XmodemPoll(XmodemData);
  if (status == TASK_FINISHED)
    XmodemData = NULL;
  return status;
}
#endif /* ENABLE_XMODEM */

/*...................................................................*/
/*       shell: Find system shell command function by name           */
/*                                                                   */
/*       Input: command = the entire command                         */
/*                                                                   */
/*      Return: pointer to command function                          */
/*...................................................................*/
static struct ShellCmd *shell(const char *command)
{
  int i;

  /* Search the command table and return if an installed command. */
  for (i = 0; ShellCommands[i].command; ++i)
  {
    /* Return the shell command if it matches. */
    if (memcmp(command, ShellCommands[i].command,
               strlen(ShellCommands[i].command)) == 0)
      return &ShellCommands[i];
  }

  /* Return command not found. */
  return NULL;
}

/*...................................................................*/
/* Global function definitions                                       */
/*...................................................................*/

/*...................................................................*/
/* ShellInit: Initialize the system shell                            */
/*                                                                   */
/*    return: zero (0)                                               */
/*...................................................................*/
int ShellInit(void)
{
  int i = 0;

#if ENABLE_XMODEM
  XmodemData = NULL;
#endif
  ShellCommands[i].command = "echo";
  ShellCommands[i].function = echo;
  ShellCommands[++i].command = "exit";
  ShellCommands[i].function = quit;
  ShellCommands[++i].command = "run";
  ShellCommands[i].function = run;
#if ENABLE_XMODEM
  ShellCommands[++i].command = "xmodem";
  ShellCommands[i].function = xmodem;
#endif
  ShellCommands[++i].command = NULL;
  ShellCommands[i].function = NULL;
  return 0;
}

/*...................................................................*/
/* ShellPoll: Periodic execution of system shell                     */
/*                                                                   */
/*     Input: data = shell state data structure                      */
/*                                                                   */
/*    return: Task state of shell (IDLE, READY, FINISHED)            */
/*...................................................................*/
int ShellPoll(void *data)
{
  static u64 command_time;
  struct shell_state *state = data;

  /* Check if command is currently executing. */
  if (state->cmd)
  {
    /* Quit and exit are special commands that do not return. */
    if ((state->cmd->function == run) ||
        (state->cmd->function == quit))
    {
      struct ShellCmd *temp = state->cmd;

      /* Clear the shell command so ShellPoll is reentrant. */
      state->i = 0;
      state->result = TASK_FINISHED;
      state->cmd = NULL;
      state->putc('\n');

      /* Execute the quit or run command, never returning. */
      temp->function((char *)state->command);
    }

    /* Execute the current command. */
    else
      state->result = state->cmd->function((char *)state->command);

    /* If execution has not finished return status. */
    if (state->result != TASK_FINISHED)
      return state->result;

    /* Calculate clock delta and return command result. */
    command_time = TimerNow() - command_time;
    putu32(command_time);

    /* Restore led timer. */
    LedTime = LedTime * 8;
    LedState.expire = TimerRegister(LedTime);
  }

  /* If execution completed then output prompt and restart state. */
  if (state->result == TASK_FINISHED)
  {
#if ENABLE_BOOTLOADER
    state->putc('b');
    state->putc('o');
    state->putc('o');
    state->putc('t');
#else
    state->putc('s');
    state->putc('h');
    state->putc('e');
    state->putc('l');
    state->putc('l');
#endif
    state->putc('>');
    state->putc(' ');
    state->i = state->result = 0;
    state->cmd = NULL;
    return TASK_IDLE;
  }

  /* If capable and character availble then read it. */
  if (state->check && state->check())
  {
    /* Read the character and echo it back to the sender. */
    state->command[state->i] = state->getc();
    state->putc(state->command[state->i]);

    /* Execute command on carriage return or end of length. */
    if ((state->command[state->i] == '\r') ||
        (state->i >= COMMAND_LENGTH - 1))
    {

      state->putc('\n');
      state->command[state->i] = 0;

      if ((state->i == 1) && (state->command[0] == '?'))
      {
        int i;

        state->puts("Available commands are:");
        for (i = 0; ShellCommands[i].command; ++i)
          state->puts(ShellCommands[i].command);
        state->putc('\n');
        state->result = TASK_FINISHED;
      }

      /* If not an empty command then look up the command structure. */
      else if (state->i)
      {
        state->cmd = shell((char *)state->command);
        if (state->cmd == NULL)
        {
          state->puts("command unknown");
          state->result = TASK_FINISHED;
        }
        else
        {
          LedTime = LedTime / 8;
          LedState.expire = TimerRegister(LedTime);
          command_time = TimerNow();
        }
      }
      else
        state->result = TASK_FINISHED;
    }
    else
      state->i++;
    return TASK_READY;
  }
  else
    return TASK_IDLE;
}

/*...................................................................*/
/* SystemShell: system shell executs commands until 'quit'           */
/*                                                                   */
/*...................................................................*/
void SystemShell(void)
{
  /* Loop to poll led and shell, executing commands.*/
  for (; ShellPoll(&Uart0State) != TASK_FINISHED;)
  {
    LedPoll(&LedState);
  }
}

#endif /* ENABLE_SHELL */

