/*...................................................................*/
/*                                                                   */
/*   Module:  fat.c                                                  */
/*   Version: 2024.0                                                 */
/*   Purpose: Interface to FAT shell commands                        */
/*                                                                   */
/*...................................................................*/
/*                                                                   */
/*                   Copyright 2024, Sean Lawless                    */
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
#include <stdio.h>
#include <string.h>
#include "sdcard.h"

#ifdef ENABLE_FAT

#include "asyncfatfs.h"

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/

static char CWD[256];  // Current Working Directory
static char CDDir[16]; // Change subdirectory in progress
static afatfsFilePtr_t openDirectory;
static afatfsFinder_t finder;
typedef enum {
    READ_INIT,
    READ_OPEN,
    READ_READ,
    READ_END
} fatState;
fatState readState = READ_INIT;

/*...................................................................*/
/* Local function definitions                                        */
/*...................................................................*/

/*...................................................................*/
/* read_directory_callback: Callback to open and read directory      */
/*                                                                   */
/*   input: file = the AsyncFATFS file pointer                       */
/*                                                                   */
/*...................................................................*/
static void read_directory_callback(afatfsFilePtr_t file)
{
  openDirectory = file;

  if (openDirectory)
  {
    if (readState == READ_OPEN)
    {
      readState = READ_READ;
      afatfs_findFirst(openDirectory, &finder);
    }

    else
    {
      afatfsOperationStatus_e status;
      fatDirectoryEntry_t *dirEntry;
      status = afatfs_findNext(openDirectory, &finder, &dirEntry);

      if (status == AFATFS_OPERATION_SUCCESS)
      {
        // If no more directory entries, move state to end directory read
        if ((dirEntry == NULL) || (dirEntry->attrib == 0))
        {
          readState = READ_END;
        }

        // If a valid file entry, convert and output the FAT name
        else if ((dirEntry->filename[0] != FAT_DELETED_FILE_MARKER) &&
                 !(dirEntry->attrib & (FAT_FILE_ATTRIBUTE_VOLUME_ID | FAT_FILE_ATTRIBUTE_SYSTEM)))
        {
          char filename[16];
          fat_convertFATStyleToFilename(dirEntry->filename, filename);
          printf("%s\n", filename);
        }
      }

      // If failure, end the read directory operation
      else if (status == AFATFS_OPERATION_FAILURE)
      {
        readState = READ_END;
        printf("Reading directory failed\n");
      }
    }
  }

  // If fopen failed, end the read directory operation
  else
  { 
    readState = READ_END;
    printf("Opening directory failed\n");
  }
}

/*...................................................................*/
/* read_file_callback: Callback to open and read a file              */
/*                                                                   */
/*   input: file = the AsyncFATFS directory file pointer             */
/*                                                                   */
/*...................................................................*/
static void read_file_callback(afatfsFilePtr_t file)
{
  openDirectory = file;
  u8 buffer[SECTOR_SIZE + 1];

  if (openDirectory)
  {
    if (readState == READ_OPEN)
      readState = READ_READ;

    uint32_t length;
    length = afatfs_fread(openDirectory, buffer, SECTOR_SIZE);

    if (length > 0)
    {
      buffer[length] = '\0';
      printf("%s", buffer);
    }
    else if (afatfs_feof(openDirectory))
    {
      puts("");
      readState = READ_END;
    }
  }

  // If fopen failed, end the read directory operation
  else
  { 
    readState = READ_END;
    printf("Opening directory failed\n");
  }
}

/*...................................................................*/
/* change_dir_callback: Callback on change directory completion      */
/*                                                                   */
/*   input: file = the AsyncFATFS directory file pointer             */
/*                                                                   */
/*...................................................................*/
static void change_dir_callback(afatfsFilePtr_t dir)
{
  if (dir)
  {
    afatfs_chdir(dir);
    afatfs_fclose(dir, NULL);
    strcat(CWD, CDDir);
    strcat(CWD, "/");
  }
  else
  {
    printf("Opening directory failed\n");
  }
}

/*...................................................................*/
/* Global function definitions                                       */
/*...................................................................*/

/*...................................................................*/
/* ReadDirectory: Set up command as a read directory state machine   */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_IDLE until complete/error, then TASK_FINISHED       */
/*...................................................................*/
int ReadDirectory(const char *command)
{
  if ((UsbUp) &&
      (afatfs_getFilesystemState() == AFATFS_FILESYSTEM_STATE_READY))
  {
    if (readState == READ_INIT)
    {
      const char *arg1 = strchr(command, ' '), *dir;

      // If argument and too long, display error and finish task
      if (arg1 && (strlen(arg1) > 16))
      {
        puts("USB not up or FAT not mounted.");
        return TASK_FINISHED;
      }

      // Otherwise if argument skip the space to directory name
      else if (arg1)
        dir = &arg1[1];

      // Otherwise use '.' for current directory
      else
        dir = ".";

      // Set read state to open and fopen the file with callback
      readState = READ_OPEN;
      afatfs_fopen(dir, "r", read_directory_callback);
    }
    else if (openDirectory && (readState == READ_READ))
      read_directory_callback(openDirectory);
  }
  else
  {
    puts("USB not up or FAT not mounted.");
    return TASK_FINISHED;
  }

  // If done close the file, clear the state variables and return finished
  if (readState == READ_END)
  {
    afatfs_fclose(openDirectory, NULL);
    openDirectory = NULL;
    readState = READ_INIT;
    return TASK_FINISHED;
  }
  else
    return TASK_IDLE;
}

/*...................................................................*/
/* PrintWorkingDirectory: Output the current working directory       */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_FINISHED                                            */
/*...................................................................*/
int PrintWorkingDirectory(const char *command)
{
  puts(CWD);
  return TASK_FINISHED;
}

/*...................................................................*/
/* ChangeDirectory: Change current working directory                 */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_FINISHED, change happens asynchronously in callback */
/*...................................................................*/
int ChangeDirectory(const char *command)
{
  if ((UsbUp) &&
      (afatfs_getFilesystemState() == AFATFS_FILESYSTEM_STATE_READY))
  {
    const char *arg1 = strchr(command, ' ');
  
    if (!arg1 || (strlen(arg1) > 16))
    {
      puts("new directory name required and 12 or less characters.");
      return TASK_FINISHED;
    }

    strcpy(CDDir, &arg1[1]);
    if ((strcmp(CDDir, "/") == 0) || (strcmp(CDDir, "..") == 0))
    {
      afatfs_chdir(NULL);
      strcpy(CWD, "/");
    }
    else
      afatfs_fopen(CDDir, "r", change_dir_callback);
  }
  else
    puts("USB not up or FAT not mounted.");
   return TASK_FINISHED;
}

// 
/*...................................................................*/
/* ReadFile: Set up command as a read file state machine             */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_IDLE until complete/error, then TASK_FINISHED       */
/*...................................................................*/
int ReadFile(const char *command)
{
  if ((UsbUp) &&
      (afatfs_getFilesystemState() == AFATFS_FILESYSTEM_STATE_READY))
  {
    if (readState == READ_INIT)
    {
      const char *arg1 = strchr(command, ' '), *file;

      // If no argument or length too long, display error and finish
      if (!arg1 || (strlen(arg1) > 16))
      {
        puts("filename required and <= 16 characters.");
        return TASK_FINISHED;
      }

      // Skip the space to file name
      file = &arg1[1];

      // Set read state to open and fopen the file with callback
      readState = READ_OPEN;
      afatfs_fopen(file, "r", read_file_callback);
    }
    else if (openDirectory && (readState == READ_READ))
      read_file_callback(openDirectory);
  }
  else
  {
    puts("USB not up or FAT not mounted.");
    return TASK_FINISHED;
  }

  // If done close the file, clear the state variables and return finished
  if (readState == READ_END)
  {
    afatfs_fclose(openDirectory, NULL);
    openDirectory = NULL;
    readState = READ_INIT;
    return TASK_FINISHED;
  }
  else
    return TASK_IDLE;
}

/*..................................................................*/
/* FatPoll: poll the FAT file system                                */
/*                                                                  */
/* returns: exit error                                              */
/*..................................................................*/
int FatPoll(void *unused)
{
  afatfs_poll();
  return TASK_IDLE;
}

/*..................................................................*/
/* FatInit: initialize the FAT file system                          */
/*                                                                  */
/*..................................................................*/
void FatInit()
{
  strcpy(CWD, "/");
  afatfs_init();
}

/*...................................................................*/
/* MountFAT: Initialize FAT file system                              */
/*                                                                   */
/*   input: command = the entire command                             */
/*                                                                   */
/*  return: TASK_FINISHED                                            */
/*...................................................................*/
int MountFAT(char *command)
{
  if (UsbUp)
  {
    // Create the polling task and initialize the FAT file system
#if ENABLE_OS
    TaskNew(MAX_TASKS - 4, FatPoll, NULL);
#endif
    FatInit();
  }
  else
    puts("USB not initialized");

  return TASK_FINISHED;
}

#endif
