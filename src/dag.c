#include "simdag/simdag.h"
#include "xbt.h"
#include "task.h"
#include "workstation.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(dag, biCPA, "Logging specific to dag");

/*
 * Get the dummy 'root' task of a DAG, i.e., the first task of the dynar.
 */
SD_task_t get_dag_root(xbt_dynar_t dag){
  SD_task_t task;
  xbt_dynar_get_cpy(dag, 0, &task);
  return task;
}

/*
 * Get the dummy 'end' task of a DAG, i.e., the last task of the dynar.
 */
SD_task_t get_dag_end(xbt_dynar_t dag){
  SD_task_t task;
  xbt_dynar_get_cpy(dag, xbt_dynar_length(dag)-1, &task);
  return task;
}

/*
 * Compute and set values of the 'bottom level' attribute of the tasks
 * that compose a DAG.
 * Rely on a top-down DFS traversal of the DAG
 *    -- bottom_level_recursive_computation(task)
 * The bottom level represents the length of the longest path from a task to
 * the 'end' task of a DAG in terms of estimated computation times. Time to
 * transfer data between tasks is not taken into account. The estimated
 * execution time of a task is included in its bottom level.
 */
void set_bottom_levels (xbt_dynar_t dag){
  unsigned int i;
  SD_task_t task;

  /* Start the DFS from the 'root' task */
  task = get_dag_root(dag);
  bottom_level_recursive_computation(task);

  /* Tasks were marked during the DFS, unmark them all */
  xbt_dynar_foreach(dag, i, task){
    SD_task_unmark(task);
  }
}

/*
 * Compute and set values of the 'top level' attribute of the tasks that
 * compose a DAG.
 * Rely on a bottom-up DFS traversal of the DAG
 *    -- top_level_recursive_computation(task)
 * The top level represents the length of the longest path from the 'root' task
 * of a DAG to a task in terms of estimated computation times. Time to
 * transfer data between tasks is not taken into account. The estimated
 * execution time of a task is not included in its top level.
 */
void set_top_levels (xbt_dynar_t dag){
  unsigned int i;
  SD_task_t task;

  /* Start the DFS from the 'end' task */
  task = get_dag_end(dag);
  top_level_recursive_computation(task);

  /* Tasks were marked during the DFS, unmark them all */
  xbt_dynar_foreach(dag, i, task){
    SD_task_unmark(task);
  }
}

/*
 * Compute and set values of the 'precedence level' attribute of the tasks
 * that compose a DAG.
 * Rely on a bottom-up DFS traversal of the DAG
 *    -- precedence_level_recursive_computation(task)
 * The precedence level represents the length of the longest path from the
 * 'root' task of a DAG to a task in terms of number of ancestor tasks. The
 * 'root' node has a precedence level value of '0'.
 */
void set_precedence_levels (xbt_dynar_t dag){
  unsigned int i;
  SD_task_t task;

  /* Start the DFS from the 'end' task */
  task = get_dag_end(dag);
  precedence_level_recursive_computation(task);

  /* Tasks were marked during the DFS, unmark them all */
  xbt_dynar_foreach(dag, i, task){
    SD_task_unmark(task);
  }
}

void set_allocations_from_iteration(xbt_dynar_t dag, int index){
  unsigned int i;
  SD_task_t task;
  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
      SD_task_set_allocation_size(task,
          SD_task_get_iterative_allocations(task, index));
      XBT_INFO("Allocation of task '%s' is set to %d (check = %d)",
          SD_task_get_name(task), SD_task_get_allocation_size(task),
          SD_task_get_iterative_allocations(task, index));
    }
  }
}

void map_allocations(xbt_dynar_t dag){
  unsigned int i, j;
  int allocation_size;
  double min_start_time,last_data_arrival, earliest_availability;
  SD_workstation_t * allocation = NULL;
  SD_task_t task, root = get_dag_root(dag);

  /* Schedule Root */
  if (SD_task_get_state(root) == SD_NOT_SCHEDULED) {
    XBT_DEBUG("Scheduling '%s'", SD_task_get_name(root));
    SD_task_schedulel(root, 1, SD_workstation_get_list());
    SD_task_set_estimated_finish_time(root, 0.0);
  }

  set_bottom_levels(dag);

  xbt_dynar_sort(dag, bottomLevelCompareTasks);

  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){

      min_start_time = SD_task_estimate_minimal_start_time(task);
      XBT_DEBUG("Task '%s' cannot start before time = %f",
          SD_task_get_name(task), min_start_time);

      allocation = get_best_workstation_set(min_start_time);
      allocation_size =  SD_task_get_allocation_size(task);
      SD_task_set_allocation(task, allocation);

      SD_task_schedulev(task, allocation_size, allocation);

      last_data_arrival = SD_task_estimate_last_data_arrival_time(task);
      earliest_availability =
          get_best_workstation_set_earliest_availability(allocation_size,
          allocation),
      SD_task_set_estimated_finish_time(task,
          MAX(last_data_arrival, earliest_availability)
          + SD_task_estimate_execution_time(task, allocation_size));

      XBT_VERB("Just scheduled task '%s' on %d workstation (first is '%s')",
          SD_task_get_name(task), allocation_size,
          SD_workstation_get_name(allocation[0]));
      XBT_VERB("   Estimated [Start-Finish] time interval = [%.3f - %.3f]",
          SD_task_get_estimated_finish_time(task)-
          SD_task_estimate_execution_time(task, allocation_size),
          SD_task_get_estimated_finish_time(task));
      for (j=0; j < allocation_size; j++){
        SD_workstation_set_available_at(allocation[j],
            SD_task_get_estimated_finish_time(task));

        /* Create resource dependencies if needed */
        if (SD_workstation_get_last_scheduled_task(allocation[j]) &&
            !SD_task_dependency_exists(
                SD_workstation_get_last_scheduled_task(allocation[j]),task))
          SD_task_dependency_add("resource", NULL,
              SD_workstation_get_last_scheduled_task(allocation[j]), task);

        SD_workstation_set_last_scheduled_task(allocation[j], task);
      }
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/
/**************               Accounting functions              **************/
/*****************************************************************************/
/*****************************************************************************/
double compute_total_work (xbt_dynar_t dag){
  unsigned int i;
  double total_work = 0.0;
  SD_task_t task;

  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
      total_work += SD_task_estimate_area(task,
          SD_task_get_allocation_size(task));
    }
  }
  return total_work;
}

void reset_simulation (xbt_dynar_t dag) {
  unsigned int i,j;
  SD_task_t task, parent;
  xbt_dynar_t parents;

  /*
   * Let's remove the resource dependencies that have been added by the
   * previous simulation round.
   */
  xbt_dynar_foreach(dag, i, task){
    parents = SD_task_get_parents(task);
    xbt_dynar_foreach(parents, j, parent){
      if (SD_task_get_kind(parent) == SD_TASK_COMP_PAR_AMDAHL){
        if (SD_task_dependency_exists(parent, task) &&
            (SD_task_dependency_get_name(parent,task)) &&
            (!strcmp(SD_task_dependency_get_name(parent,task), "resource"))){
          XBT_DEBUG("Remove resource dependency between tasks '%s' and '%s'",
              SD_task_get_name(parent), SD_task_get_name(task));
          SD_task_dependency_remove(parent, task);
        }
      }
    }
  }
  reset_workstation_attributes();
  SD_application_reinit();
  set_bottom_levels(dag);
}
