/*...................................................................*/
/*                                                                   */
/*   Module:  os.c                                                   */
/*   Version: 2015.0                                                 */
/*   Purpose: priority loop scheduler based Operating System         */
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

#if ENABLE_OS

/*...................................................................*/
/* Configuration                                                     */
/*...................................................................*/
#define USE_HIGHER_PRIORITY     TRUE /* true to increase priority  */
                                     /* until a task is available. */

/*...................................................................*/
/* Global Variables                                                  */
/*...................................................................*/
struct task Tasks[MAX_TASKS];

/*...................................................................*/
/* Global Function Definitions                                       */
/*...................................................................*/

/*...................................................................*/
/*    TaskNew: Create a new task                                     */
/*                                                                   */
/*      Input: priority the importance of the task                   */
/*             poll the function to execute for the new task         */
/*             data is the state data structure for the new task     */
/*                                                                   */
/*    Returns: the resulting priority or less than zero if error     */
/*...................................................................*/
int TaskNew(int priority, int (*poll) (void *data), void *data)
{
  /* Return if priority greater than allowed */
  if (priority >= MAX_TASKS)
  {
    puts("NewTask failed: priority greater than maximum.");
    return -1;
  }

#if USE_HIGHER_PRIORITY
  /* Increase priority until a task is available */
  /* if poll is NULL but data valid the task is disabled so skip */
  while ((Tasks[priority].poll != NULL) &&
         (Tasks[priority].data != NULL))
  {
    if (++priority >= MAX_TASKS)
    {
      puts("NewTask failed: non availble, including higher priority.");
      return -1;
    }
  }
#endif

  /* Set up the task to poll and return success */
  Tasks[priority].data = data;
  Tasks[priority].poll = poll;
  return priority;
}

/*...................................................................*/
/*    TaskEnd: End an existing task                                  */
/*                                                                   */
/*      Input: priority the importance of the task                   */
/*                                                                   */
/*    Returns: the priority or less than zero if error               */
/*...................................................................*/
int TaskEnd(int priority)
{
  /* Return if priority greater than allowed or not in use */
  if ((priority >= MAX_TASKS) || (Tasks[priority].poll == NULL))
    return -1;

  /* Remove the polling for this task and return success */
  Tasks[priority].data = NULL;
  Tasks[priority].poll = NULL;
  return priority;
}

/*...................................................................*/
/*  OsInit: initialize the operating scheduler                       */
/*...................................................................*/
void OsInit(void)
{
  bzero(Tasks, sizeof(struct task) * MAX_TASKS);
}


/*...................................................................*/
/*  OsTick: tick or step through the operating scheduler once        */
/*                                                                   */
/* returns: exit error                                               */
/*...................................................................*/
int OsTick(void)
{
  int status, taskid;

  /* Set status to invalid value to check if a task exists. */
  status = -1;

  /* Execute tasks in order of priority. */
  for (taskid = 0; taskid < MAX_TASKS; taskid++)
  {
    /* Execute the task poll function if present. */
    if (Tasks[taskid].poll)
    {
      /* Execute the task minimally, saving state before returning. */
      status = Tasks[taskid].poll(Tasks[taskid].data);

      /* If ready, break out of loop to reexecute high priority. */
      if (status == TASK_READY)
        break;

      /* Stop and free this task in the list if finished. */
      if (status == TASK_FINISHED)
        Tasks[taskid].poll = NULL;
    }

    /* Otherwise continue to execute next priority task. */
  }

  return status;
}

/*...................................................................*/
/* OsStart: continually execute the operating scheduler              */
/*...................................................................*/
void OsStart(void)
{
  int status = TASK_IDLE;

  /* Execute all tasks until none remain to execute. */
  for (; status != -1; )
    status = OsTick();
}

#endif
