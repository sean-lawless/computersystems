/*...................................................................*/
/*                                                                   */
/*   Module:  main.c                                                 */
/*   Version: 2015.0                                                 */
/*   Purpose: main function for console application                  */
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
#include <stdlib.h>
#include <string.h>
#include <usb/request.h>
#include <usb/device.h>

/*...................................................................*/
/* Global variables                                                  */
/*...................................................................*/
extern int OgSp;
int ScreenUp, UsbUp, NetUp;

#if ENABLE_USB_HID

static char KeyIn;

unsigned int usbKbdCheck(void)
{
  return KeyIn;
}

char usbKbdGetc(void)
{
  char key = KeyIn;

  KeyIn = 0;
  return key;
}

void KeyPressedHandler(const char ascii)
{
  KeyIn = ascii;

  /* If interactive console is not up yet, output to UART. */
  if (ConsoleState.getc == NULL)
    Uart0State.putc(ascii);
}

#endif /* ENABLE_USB_HID */

#if ENABLE_USB
int UsbHostStart(char *command)
{
  if (!UsbUp)
  {
    RequestInit();
    DeviceInit();

    if (!HostEnable())
    {
      puts("Cannot initialize USB host controller interface");
      return TASK_FINISHED;
    }
    UsbUp = TRUE;
  }
  else
    puts("USB host already initialized");

  return TASK_FINISHED;
}

#endif /* ENABLE_USB */

#if ENABLE_USB_STORAGE

struct partition
{
  u8 status; // 0x80 is bootable/active
  u8 chs_first_head, chs_first_sector, chs_first_cylinder;
  u8 type;
  u8 chs_last_head, chs_last_sector, chs_last_cylinder;
  u32 first_lba;
  u32 num_sectors;
};

#define SECTOR_SIZE 512

u8 ReadSector[SECTOR_SIZE];
i8 MountedPartition = 0;
i8 PartitionToMount = -1;
struct partition partitions[4];

void read_sector_callback(u8 *buffer, int buffLen)
{
  int i;

  // Output error and return if read failed
  if (buffLen < 0)
  {
    printf("Read sector failed, CSW status %d\n", -buffLen);
    return;
  }
  else if (buffLen < SECTOR_SIZE)
    puts("read_sector_callback: short read, but parsing anyway... ");

  // If we read the boot sector (offset 0)
  if (MassStorageOffset() == 0)
  {
    // Check boot signature
    if ((buffer[510] != 0x55) || (buffer[511] != 0xAA))
      printf("Invalid boot signature!");

/*
    // Display the Master Boot Record (MBR)
    puts("Master Boot Record (MBR) =>");
    for (i = 0; i + 8 <= buffLen; i += 8)
    {
      printf("  0x%x =>  %x:%x:%x:%x|%x:%x:%x:%x\n", i,
             buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3],
             buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7]);
    }
*/

    // copy the partitions from the MBR
    memcpy(&partitions[0], &buffer[0x1BE], 16);
    memcpy(&partitions[1], &buffer[0x1CE], 16);
    memcpy(&partitions[2], &buffer[0x1DE], 16);
    memcpy(&partitions[3], &buffer[0x1EE], 16);
    
    for (i = 0; i < 4; ++i)
    {
      // If a valid partition output details and mount
      if (partitions[i].type > 0)
      {
        printf("  Partition %x (status %x):\n", i + 1,
               partitions[i].status);
        printf("    First head %d, sector %d, cylinder %d\n",
               partitions[i].chs_first_head,
               partitions[i].chs_first_sector,
               partitions[i].chs_first_cylinder);
        printf("    Partition type 0x%x\n", partitions[i].type);
        printf("    Last head %d, sector %d, cylinder %d\n",
               partitions[i].chs_last_head,
               partitions[i].chs_last_sector,
               partitions[i].chs_last_cylinder);
        printf("    First LBA %d\n", partitions[i].first_lba);
        printf("    Number of Sectors %d (%d MBytes)\n",
             partitions[i].num_sectors,
             (partitions[i].num_sectors * SECTOR_SIZE) / (1024 * 1024));

        // Mount the first partition found if not user selected
        if ((MountedPartition == 0) && ((PartitionToMount == 0) ||
            (PartitionToMount == i + 1)))
        {
          printf("Partition %d mounted!\n", i + 1);
          MountedPartition = i + 1;
        }
        else
          printf("  Skip mount for partition %d (%d, %d)...\n", i + 1,
                 MountedPartition, PartitionToMount);
      }
      else if (partitions[i].status > 0)
        printf("  Partition %x is empty", i);
    }
  }
  else
  {
    // Otherwise we read a normal sector
    u64 offset = MassStorageOffset();

    // Display the sector read
    printf("  Read bytes from offset %d:\n", (int)offset);
    for (i = 0; i + 8 <= buffLen; i += 8)
    {
      printf("  0x%x =>  %x:%x:%x:%x|%x:%x:%x:%x\n", (int)offset + i,
             buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3],
             buffer[i + 4], buffer[i + 5], buffer[i + 6], buffer[i + 7]);
    }

    // TODO: pass to FAT parser?
  }
}

int MountFilesystem(char *command)
{
  if (UsbUp)
  {
    const char *arg1 = strchr(command, ' ');
    
    // Read the partition number from the command
    if (arg1 != NULL)
      PartitionToMount = atoi(++arg1); // skip past space
    else
      PartitionToMount = 0;

    // Seek to first block, the MBR
    MassStorageSeek(0);

    // Read the Master Boot Record (MBR)
    if (MassStorageRead(ReadSector, 512, read_sector_callback) <= 0)
      puts("MassStorageRead failed");
  }
  else
    puts("USB not initialized");

  return TASK_FINISHED;
}

int ReadFilesystem(const char *command)
{
  if (UsbUp)
  {
    const char *arg1 = strchr(command, ' ');
    u32 block;
    
    if (arg1 != NULL)
    {
      block = atoi(++arg1); // skip past space
      printf("Seeking to block %d (%s)\n", (int)block, arg1);
      u64 offset = block * SECTOR_SIZE;

      // Seek to the user specific offset
      if (MassStorageSeek(offset) != offset)
      {
        puts("Seek failed");
        return TASK_FINISHED;
      }
    }

    // Read the sector
    if (MassStorageRead(ReadSector, 512, read_sector_callback) <= 0)
      puts("MassStorageRead failed");
  }
  else
    puts("USB not initialized");

  return TASK_FINISHED;
}

#endif

/*...................................................................*/
/*        main: Application Entry Point                              */
/*                                                                   */
/*     Returns: Exit error                                           */
/*...................................................................*/
int main(void)
{
  // Initialize global variables
  NetUp = UsbUp = ScreenUp = FALSE;

  /* Initialize the hardware. */
  BoardInit();

#if ENABLE_OS
  // Initialize the Operating System (OS) and create system tasks
  OsInit();

#if ENABLE_UART0
  /* Set task specific stdio. */
  StdioState = &Uart0State;
  TaskNew(0, ShellPoll, &Uart0State);
#elif ENABLE_UART1
  StdioState = &Uart1State;
  TaskNew(0, ShellPoll, &Uart1State);
#endif

  // Initialize the timer and LED tasks
  TaskNew(1, TimerPoll, &TimerStart);
  TaskNew(MAX_TASKS - 1, LedPoll, &LedState);
#endif /* ENABLE_OS */

#if ENABLE_AUTO_START
#if ENABLE_VIDEO
  /* Initialize screen and console. */
  ScreenInit();

  /* Register video console with the OS. */
  bzero(&ConsoleState, sizeof(ConsoleState));
  Console(&ConsoleState);
#endif
#if ENABLE_USB
  // Start the USB host
  UsbHostStart(NULL);
#endif
#endif

  /* Display the introductory splash. */
  puts("Console application");
  putu32(OgSp);
  puts(" : stack pointer");

#if ENABLE_OS
  /* Run the priority loop scheduler. */
  OsStart();
#elif ENABLE_SHELL
  /* Run the system console shell. */
  SystemShell();
#else
  puts("Press any key to exit application");
  Uart0Getc();
#endif

  /* On OS exit say goodbye. */
  puts("Goodbye");
  return 0;
}
