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
/* Global Variables                                                  */
/*...................................................................*/
struct task Tasks[MAX_TASKS], *TasksStart;
int TaskId;

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
struct task *TaskNew(int priority, int (*poll) (void *data),void *data)
{
  int i;
  struct task *current, *newTask;

  /* Find a free task. */
  for (i = 0; i < MAX_TASKS; ++i)
  {
    if ((Tasks[i].poll == NULL) &&
        (Tasks[i].data == NULL))
      break;
  }

  /* If no tasks available, return failure. */
  if (i >= MAX_TASKS)
    return NULL;
  else
    newTask = &Tasks[i];

  /* Set up the task to poll and return success */
  newTask->data = data;
  newTask->poll = poll;
  newTask->stdio = StdioState;
  newTask->priority = priority;
  newTask->list.next = NULL;
  newTask->list.previous = NULL;
  if (!newTask->stdio)
  {
#if ENABLE_UART0
    newTask->stdio = &Uart0State;
#elif ENABLE_UART1
    newTask->stdio = &Uart1State;
#endif
  }

  // If task list is empty start it with this node
  if (TasksStart == NULL)
  {
    TasksStart = newTask;
    return newTask;
  }

  // Search list of tasks in priority order
  for (current = (void *)TasksStart; current;
       current = (void *)current->list.next)
  {
    /* If priority is less for this entry, insert before here. */
    if (priority < current->priority)
    {
      if (current == TasksStart)
        TasksStart = &Tasks[i];
      ListInsertBefore(&Tasks[i], current);
      break;
    }

    // Otherwise if this is the last node insert after here
    else if (current->list.next == NULL)
    {
      ListInsertAfter(&Tasks[i], current);
      break;
    }
  }

  return newTask;
}

/*...................................................................*/
/*    TaskEnd: End an existing task                                  */
/*                                                                   */
/*      Input: priority the importance of the task                   */
/*                                                                   */
/*    Returns: zero on success or -1 on error                        */
/*...................................................................*/
int TaskEnd(struct task *endingTask)
{
  /* Return if priority greater than allowed or not in use */
  if ((endingTask == NULL) || (endingTask->poll == NULL))
    return -1;

  /* Remove the polling for this task and return success */
  endingTask->data = NULL;
  endingTask->poll = NULL;
  endingTask->stdio = NULL;
  ListRemove(endingTask->list);
  return 0;
}

/*...................................................................*/
/*  OsInit: initialize the operating scheduler                       */
/*...................................................................*/
void OsInit(void)
{
  // Initialize system timers
  TimerInit();

  // Initilize the tasks list
  TasksStart = NULL;

  // Initialize system tasks
  bzero(Tasks, sizeof(struct task) * MAX_TASKS);
  TaskId = 0;
}


/*...................................................................*/
/*  OsTick: tick or step through the operating scheduler once        */
/*                                                                   */
/* returns: exit error                                               */
/*...................................................................*/
int OsTick(void)
{
  int status;
  struct task *currentTask;

  /* Set status to invalid value to check if a task exists. */
  status = -1;

  /* Execute tasks in order of priority. */
  for (currentTask = (void *)TasksStart; currentTask;
       currentTask = (void *)currentTask->list.next)
  {
    /* Execute the task poll function if present. */
    if (currentTask->poll)
    {
      /* Set task specific stdio. */
      if (currentTask->stdio)
        StdioState = currentTask->stdio;

      /* Execute the task minimally, saving state before returning. */
      status = currentTask->poll(currentTask->data);

      /* If ready, break out of loop to reexecute high priority. */
      if (status == TASK_READY)
        break;

      /* Stop and free this task in the list if finished. */
      if (status == TASK_FINISHED)
        currentTask->poll = NULL;
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
  TaskId = -1;
  for (; status != -1; )
    status = OsTick();
}

#endif
