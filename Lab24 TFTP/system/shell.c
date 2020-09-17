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
#include <stdlib.h>
#include <system.h>
#include <board.h>
#if ENABLE_GAME
#include <game_grid.h>
#endif
#if ENABLE_USB
#include <usb/request.h>
#include <usb/device.h>
#endif

#if ENABLE_SHELL

#define MAX_SHELL_COMMANDS     24

/*
** Shell Functions
*/
/* external commands */
#if ENABLE_USB
extern int UsbHostStart(const char *command);
#if ENABLE_USB_HID
extern int KeyboardUp(const char *command);
extern int MouseUp(const char *command);
#endif
#endif /* ENABLE_USB */
#if ENABLE_NETWORK
//extern int Echo(char *command);
extern int NetStart(const char *command);
extern int Tftp(const char *command);
#endif

/* local commands */
#if ENABLE_XMODEM
static int xmodem(const char *command);
#endif
#if ENABLE_VIDEO
extern int ClearDisplay(const char *command);
#endif
static int echo(const char *command);
static int run(const char *command);
static int quit(const char *command);
static int rboot(const char *command);

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
static struct ShellCmd ShellCommands[MAX_SHELL_COMMANDS];
struct shell_state Uart0State, Uart1State, ConsoleState;

/*...................................................................*/
/* Local function definitions                                        */
/*...................................................................*/

/*...................................................................*/
/*   rboot: Reboot the hardware system (hardware reboot)             */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_FINISHED                                            */
/*...................................................................*/
static int rboot(const char *command)
{
  SystemReboot();
  return TASK_FINISHED;
}

/*...................................................................*/
/* Local function definitions                                        */
/*...................................................................*/

#if ENABLE_VIDEO
/*...................................................................*/
/* screen_on: Initialize and activate video screen and console       */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_FINISHED                                            */
/*...................................................................*/
static int screen_on(const char *command)
{
  if (!ScreenUp)
  {
    /* Initialize screen and console. */
    ScreenInit();

    /* Register video console with the OS. */
    Console(&ConsoleState);
  }
  else
    puts("Screen already activated");
  return TASK_FINISHED;

}

/*...................................................................*/
/* screen_clear: Clear the screen (ie all pixels black)              */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_FINISHED                                            */
/*...................................................................*/
static int screen_clear(const char *command)
{
  if (ScreenUp)
  {
    /* Clear the screen. */
    ScreenClear();
  }
  else
    puts("Screen not active");
  return TASK_FINISHED;

}
#endif

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
#if ENABLE_UART0
#if ENABLE_SHELL
  /* Display on UART0. */
  if (Uart0State.puts)
    Uart0State.puts(command);
  else
#endif
    Uart0Puts(command);
#endif
#if ENABLE_UART1
#if ENABLE_SHELL
  /* Display on UART1. */
  if (Uart1State.puts)
    Uart1State.puts(command);
  else
#endif
    Uart1Puts(command);
#endif
#if ENABLE_VIDEO
  /* Display on video screen. */
  if (ScreenUp)
    ConsoleState.puts(command);
#endif
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
#if RPI == 1
  u32 rpi = 0xc42; /* RPI1 hw id as required for Linux kernel boot */
#elif RPI == 2
  u32 rpi = 0xc43; /* RPI2 hw id as required for Linux kernel boot */
#else
  u32 rpi = 0xc44; /* RPI3 hw id as required for Linux kernel boot */
#endif

#if ENABLE_VIDEO
  if (ScreenUp)
  {
    ScreenClose();
    ScreenUp = FALSE;
  }
#endif
#if ENABLE_USB
  if (UsbUp)
  {
    HostDisable();
    RequestInit();
    DeviceInit();
    UsbUp = FALSE;
  }
#endif

  /* assign the machine ID to register one (r1) for other kernels */
  asm volatile("mov r1, %0" : : "r" (rpi));
  /* what else? why does linux complain about memory size? */
  /* Maybe clear all the memory used by bootloader? */

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
#if ENABLE_VIDEO
  // Turn off the screen if up
  if (ScreenUp)
  {
    ScreenClose();
    ScreenUp = FALSE;
  }
#endif
#if ENABLE_USB
  if (UsbUp)
  {
    HostDisable();
    RequestInit();
    DeviceInit();
    UsbUp = FALSE;
  }
#endif

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
  ShellCommands[++i].command = "reboot";
  ShellCommands[i].function = rboot;
#if ENABLE_USB
  ShellCommands[++i].command = "Usb";
  ShellCommands[i].function = UsbHostStart;
#endif
#if ENABLE_USB_HID
  ShellCommands[++i].command = "Keyboard";
  ShellCommands[i].function = KeyboardUp;
  ShellCommands[++i].command = "mouse";
  ShellCommands[i].function = MouseUp;
#endif
#if ENABLE_NETWORK
  ShellCommands[++i].command = "net";
  ShellCommands[i].function = NetStart;
  ShellCommands[++i].command = "tftp";
  ShellCommands[i].function = Tftp;
#endif
#if ENABLE_VIDEO
  ShellCommands[++i].command = "screen";
  ShellCommands[i].function = screen_on;
  ShellCommands[++i].command = "clear";
  ShellCommands[i].function = screen_clear;
#endif
#if ENABLE_GAME
  ShellCommands[++i].command = "game";
  ShellCommands[i].function = GameStart;

  ShellCommands[++i].command = "i";
  ShellCommands[i].function = North;
  ShellCommands[++i].command = ",";
  ShellCommands[i].function = South;
  ShellCommands[++i].command = "l";
  ShellCommands[i].function = East;
  ShellCommands[++i].command = "j";
  ShellCommands[i].function = West;

  ShellCommands[++i].command = "o";
  ShellCommands[i].function = NorthEast;
  ShellCommands[++i].command = "u";
  ShellCommands[i].function = NorthWest;
  ShellCommands[++i].command = ".";
  ShellCommands[i].function = SouthEast;
  ShellCommands[++i].command = "m";
  ShellCommands[i].function = SouthWest;

  ShellCommands[++i].command = " ";
  ShellCommands[i].function = GamePause;
  ShellCommands[++i].command = "k";
  ShellCommands[i].function = Action;
#endif
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
  struct shell_state *std;

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
    std = StdioState;
    StdioState = state;
    putu32(command_time);
    StdioState = std;

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
    state->command[state->i + 1] = '\0';

    /* Execute command on carriage return or end of command/length. */
    if ((state->command[state->i] == '\r') ||
        (state->command[state->i] == '\n') ||
        (state->i >= COMMAND_LENGTH - 1) ||
        ((state->i == 0) && (shell((char *)state->command))))
    {
      if (state->command[state->i] == '\r')
        state->command[state->i] = 0;
      state->putc('\n');
      if (state->i == 0)
        state->putc('\r');

      if ((state->i <= 1) && (state->command[0] == '?'))
      {
        int i;

        state->puts("Available commands are:");
        for (i = 0; ShellCommands[i].command; ++i)
          state->puts(ShellCommands[i].command);
        state->putc('\n');
        state->result = TASK_FINISHED;
      }

      /* If not an empty command then look up the command structure. */
      else if (state->i || state->command[state->i])
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
    else if (state->command[state->i] == '\b')
    {
      state->putc(' ');
      state->putc('\b');
      state->i--;
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
  /* Loop to poll the shell, accepting commands and executing them. */
  for (; ShellPoll(&Uart0State) != TASK_FINISHED;)
    ;
}

#endif /* ENABLE_SHELL */

