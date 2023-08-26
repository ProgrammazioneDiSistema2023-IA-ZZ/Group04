/// @file scheduler_algorithm.c
/// @brief Round Robin algorithm.
/// @copyright (c) 2014-2022 This file is distributed under the MIT License.
/// See LICENSE.md for details.
/// Scheduler algorithms completion/implementation by matitaviola (except RR)

// Include the kernel log levels.
#include "sys/kernel_levels.h"
/// Change the header.
#define __DEBUG_HEADER__ "[SCHALG]"
/// Set the log level.
#define __DEBUG_LEVEL__ LOGLEVEL_NOTICE

#include "hardware/timer.h"
#include "process/prio.h"
#include "io/debug.h"
#include "assert.h"
#include "klib/list_head.h"
#include "process/wait.h"
#include "process/scheduler.h"
#include "limits.h"

/// @brief Updates task execution statistics.
/// @param task the task to update.
static void __update_task_statistics(task_struct *task);

/// @brief Checks if the given task is actually a periodic task.
/// @param task the task to check.
/// @return true if the task is periodic, false otherwise.
static inline bool_t __is_periodic_task(task_struct *task)
{
    // Check if the task is a periodic one and it is not under analysis.
    return task->se.is_periodic && !task->se.is_under_analysis;
}

/// @brief Employs time-sharing, giving each job a timeslice, and is also
/// preemptive since the scheduler forces the task out of the CPU once
/// the timeslice expires.
/// @param runqueue list of all processes.
/// @param skip_periodic tells the algorithm if there are periodic processes in
/// the list, and in that case it needs to skip them.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_rr(runqueue_t *runqueue, bool_t skip_periodic)
{
    // If there is just one task, return it; no need to do anything.
    if (list_head_size(&runqueue->curr->run_list) <= 1) {
        return runqueue->curr;
    }
    // Search for the next task (we do not start from the head, so INSIDE, skip the head).
    list_for_each_decl(it, &runqueue->curr->run_list)
    {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue)
            continue;
        // Get the current entry.
        task_struct *entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes
        if (entry->state != TASK_RUNNING)
            continue;
        // If entry is a periodic task, and we were asked to skip periodic tasks, skip it.
        if (__is_periodic_task(entry) && skip_periodic)
            continue;
        // We have our next entry.
        return entry;
    }
    return NULL;
}

/// @brief Is a non-preemptive algorithm, where each task is assigned a
/// priority. Processes with highest priority are executed first, while
/// processes with same priority are executed on first-come/first-served basis.
/// Priority can be decided based on memory requirements, time requirements or
/// any other resource requirement.
/// @param runqueue list of all processes.
/// @param skip_periodic tells the algorithm if there are periodic processes in
/// the list, and in that case it needs to skip them.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_priority(runqueue_t *runqueue, bool_t skip_periodic)
{
#ifdef SCHEDULER_PRIORITY
    // Get the first element of the list.
    task_struct *next = list_entry(runqueue->queue.next, struct task_struct, run_list);

    // Get its static priority.
    time_t min = next->se.prio;

    // If there is just one task, return it; no need to do anything.
    if (list_head_size(&runqueue->curr->run_list) <= 1) {
        return runqueue->curr;
    }

    // Search for the task with the smallest static priority.
    list_for_each_decl(it, &runqueue->curr->run_list)
    {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue)
            continue;
        // Get the current entry.
        task_struct *entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes
        if (entry->state != TASK_RUNNING)
            continue;
        // If entry is a periodic task, and we were asked to skip periodic tasks, skip it.
        if (__is_periodic_task(entry) && skip_periodic)
            continue;
        // Check if the entry has a lower priority.
        if (entry->se.prio <= min) {
                next = entry;
                min = entry->se.prio;
        }
    }
    return next;
#else
    return __scheduler_rr(runqueue, skip_periodic);
#endif
}


/// @brief It aims at giving a fair share of CPU time to processes, and achieves
/// that by associating a virtual runtime to each of them. It always tries to
/// run the task with the smallest vruntime (i.e., the task which executed least
/// so far). It always tries to split up CPU time between runnable tasks as
/// close to "ideal multitasking hardware" as possible.
/// @param runqueue list of all processes.
/// @param skip_periodic tells the algorithm if there are periodic processes in
/// the list, and in that case it needs to skip them.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_cfs(runqueue_t *runqueue, bool_t skip_periodic)
{

    // Get the first element of the list.
    task_struct *next = list_entry(runqueue->queue.next, struct task_struct, run_list);


    // Get its virtual runtime.
    time_t min = next->se.vruntime;

    // If there is just one task, return it; no need to do anything.
    if (list_head_size(&runqueue->curr->run_list) <= 1) {
        return runqueue->curr;
    }

    // Search for the task with the smallest vruntime value.
    list_for_each_decl(it, &runqueue->curr->run_list)
    {
        // Check if we reached the head of list_head, and skip it.
        if (it == &runqueue->queue)
            continue;
        // Get the current entry.
        task_struct *entry = list_entry(it, task_struct, run_list);
        // We consider only runnable processes
        if (entry->state != TASK_RUNNING)
            continue;
        // If entry is a periodic task, and we were asked to skip periodic tasks, skip it.
        if (__is_periodic_task(entry) && skip_periodic)
            continue;

        // Check if the element in the list has a smaller vruntime value.
        if (entry->se.vruntime < min) {
            next = entry;
            min = entry->se.vruntime;
        }
    }
    return next;
}

/// @brief Executes the task with the earliest absolute deadline among all the
/// ready tasks.
/// @param runqueue list of all processes.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_aedf(runqueue_t *runqueue)
{
    //pointer to the next task
    task_struct *next = NULL, *entry;

    //the next deadline, starting from the maximum possible one
    time_t next_dl = UINT_MAX;

    list_for_each_decl(it, &runqueue->queue){

        //if we're at the head, we skip it
        if(it == &runqueue->queue)
            continue;

        entry = list_entry(it, task_struct, run_list);

        //check that it's not an entry with deadline 0
        if(entry->se.deadline != 0){
            if(entry->se.deadline <= next_dl){
                next = entry;
                next_dl = next->se.deadline;
            }
        }
    }

    //then if i haven't found a valid "real time" task, i use the CFS
    if(next == NULL)
        next = __scheduler_cfs(runqueue, true); //true = skips periodic tasks

    return next;
    
}

/// @brief Executes the task with the earliest absolute DEADLINE among all the
/// ready tasks. When a task was executed, and its period is starting again, it
/// must be set as 'executable again', and its deadline and next_period must be
/// updated.
/// @param runqueue list of all processes.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_edf(runqueue_t *runqueue)
{
    //pointer to the next task
    task_struct *next = NULL, *entry;

    //the next deadline, starting from the maximum possible one
    time_t next_dl = UINT_MAX;

    //iterate over the tasks list looking for the mimimum deadline
    list_for_each_decl(it, &runqueue->queue){

        //if we're at the head, we skip it
        if(it == &runqueue->queue)
            continue;
        
        //gets the task_struct from the list node
        entry = list_entry(it, task_struct, run_list);

        //we skip non-period tasks or a periodic task that's still undergoing schedulability analysis
        if(!entry->se.is_periodic || entry->se.is_under_analysis)
            continue;
        
        /*
        if the entry has already been executed and the time period is starting again
        I mark it as executable again and update the periods
        */
        if(entry->se.executed && (entry->se.next_period <= timer_get_ticks())){

            entry->se.executed = false;
            entry->se.deadline += entry->se.period;
            entry->se.next_period += entry->se.period;

        }//if it's not marked as executed I check if it's the closest deadline
        else if(!entry->se.executed && (entry->se.deadline < next_dl)){
            next = entry;
            next_dl = next->se.deadline;
        }

    }

    //then if i haven't found a valid periodic task, i use the CFS
    if(next == NULL)
        next = __scheduler_cfs(runqueue, true); //true = skips periodic tasks

    return next;
}

/// @brief Executes the task with the earliest next PERIOD among all the ready
/// tasks.
/// @details When a task was executed, and its period is starting again, it must
/// be set as 'executable again', and its deadline and next_period must be
/// updated.
/// @param runqueue list of all processes.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_rm(runqueue_t *runqueue)
{
    //pointer to the next task
    task_struct *next = NULL, *entry;

    //the next period, starting from the maximum possible one
    time_t next_np = UINT_MAX;

    //iterate over the tasks list looking for the closest next period
    list_for_each_decl(it, &runqueue->queue){

        //if we're at the head, we skip it
        if(it == &runqueue->queue)
            continue;
        
        //gets the task_struct from the list node
        entry = list_entry(it, task_struct, run_list);

        //we skip non-period tasks or a periodic task that's still undergoing schedulability analysis
        if(!entry->se.is_periodic || entry->se.is_under_analysis)
            continue;

        if(entry->se.executed && (entry->se.next_period <= timer_get_ticks())){

            entry->se.executed = false;
            entry->se.deadline += entry->se.period;
            entry->se.next_period += entry->se.period;

        }//if it's not marked as executed I check if it's the closest deadline
        else if(!entry->se.executed && (entry->se.next_period < next_np)){
            next = entry;
            next_np = next->se.next_period;
        }
    }

    //then if i haven't found a valid periodic task, i use the CFS
    if(next == NULL)
        next = __scheduler_cfs(runqueue, true); //true = skips periodic tasks

    return next;
}

/// @brief Executes the task with the least laxity among all the ready
/// tasks.
/// @details LLF is an algorithm that considers the "laxity" of tasks, 
/// which is the difference between a task's deadline and its remaining execution time. 
/// The task with the least laxity is given the highest priority. 
/// This approach aims to minimize the number of missed deadlines.
/// @param runqueue list of all processes.
/// @return the next task on success, NULL on failure.
static inline task_struct *__scheduler_llf(runqueue_t *runqueue)
{
    //pointer to the next task
    task_struct *next = NULL, *entry;

    //the next period, starting from the maximum possible one
    time_t min_lax = UINT_MAX;

    //iterate over the tasks list looking for the closest next period
    list_for_each_decl(it, &runqueue->queue){

        //if we're at the head, we skip it
        if(it == &runqueue->queue)
            continue;
        
        //gets the task_struct from the list node
        entry = list_entry(it, task_struct, run_list);

        //we skip non-period tasks or a periodic task that's still undergoing schedulability analysis
        if(!entry->se.is_periodic || entry->se.is_under_analysis)
            continue;

        if(entry->se.executed && (entry->se.next_period <= timer_get_ticks())){

            entry->se.executed = false;
            entry->se.deadline += entry->se.period;
            entry->se.next_period += entry->se.period;

        }//if it's not marked as executed I check if it's the least laxed
        else if(!entry->se.executed){
            //get's how much time till the deadline
            //laxity = remaining time - total execution time up to now
            int this_lax = (entry->se.deadline - timer_get_ticks()) - entry->se.sum_exec_runtime;
            if(this_lax < min_lax){
                next = entry;
                min_lax = this_lax;
            }
        }
    }

    //then if i haven't found a valid real time task, i use the CFS
    if(next == NULL)
        next = __scheduler_cfs(runqueue, true); //true = skips periodic tasks

    return next;
}


task_struct *scheduler_pick_next_task(runqueue_t *runqueue)
{
    // Update task statistics.
#if (defined(SCHEDULER_CFS) || defined(SCHEDULER_EDF) || defined(SCHEDULER_RM) || defined(SCHEDULER_AEDF) || defined(SCHEDULER_LLF))
    __update_task_statistics(runqueue->curr);
#endif

    // Pointer to the next task to schedule.
    task_struct *next = NULL;
#if defined(SCHEDULER_RR)
    next = __scheduler_rr(runqueue, false);
#elif defined(SCHEDULER_PRIORITY)
    next = __scheduler_priority(runqueue, false);
#elif defined(SCHEDULER_CFS)
    next = __scheduler_cfs(runqueue, false);
#elif defined(SCHEDULER_EDF)
    next = __scheduler_edf(runqueue);
#elif defined(SCHEDULER_RM)
    next = __scheduler_rm(runqueue);
#elif defined(SCHEDULER_AEDF)
    next = __scheduler_aedf(runqueue);
#elif defined(SCHEDULER_LLF)
    next = __scheduler_llf(runqueue);
#else
#error "You should enable a scheduling algorithm!"
#endif

    assert(next && "No valid task selected by the scheduling algorithm.");

    // Update the last context switch time of the next task.
    next->se.exec_start = timer_get_ticks();

    return next;
}

static void __update_task_statistics(task_struct *task)
{
    // See `prio.h` for more support functions.
    assert(task && "Current task is not valid.");

    // While periodic task is under analysis is executed with aperiodic
    // scheduler and can be preempted by a "true" periodic task.
    // We need to sum all the execution spots to calculate the WCET even
    // if is a more pessimistic evaluation.
    // Update the delta exec.
    task->se.exec_runtime = timer_get_ticks() - task->se.exec_start;

    // Perform timer-related checks.
    update_process_profiling_timer(task);

    // Set the sum_exec_runtime.
    task->se.sum_exec_runtime += task->se.exec_runtime;

    // If the task is not a periodic task we have to update the virtual runtime.
    if (!task->se.is_periodic) {
        // Get the weight of the current task.
        time_t weight = GET_WEIGHT(task->se.prio);
        // If the weight is different from the default load, compute it.
        if (weight != NICE_0_LOAD) {
            // Get the multiplicative factor for its delta_exec.
            double factor = ((double)NICE_0_LOAD)/((double)weight);
            // Weight the delta_exec with the multiplicative factor.
            task->se.exec_runtime = (int)(((double)task->se.exec_runtime)*factor);
        }
        // Update vruntime of the current task.
        task->se.vruntime += task->se.exec_runtime;
    }
}
