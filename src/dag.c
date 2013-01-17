#include "simdag/simdag.h"
#include "xbt.h"
#include "task.h"
#include "workstation.h"


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

double compute_total_work (xbt_dynar_t dag){
  unsigned int i;
  double total_work = 0.0;
  SD_task_t task;

  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
      total_work += 0; //TODO getArea(task, SD_task_get_nworkstations(task));
    }
  }
  return total_work;
}

void reset_simulation (xbt_dynar_t dag) {
  //TODO check if not better to add another parameter to indicate which
  //     simulated time value to use to reset some attributes
  reset_workstation_attributes();
  SD_application_reinit();
  //TODO check how to reset task scheduling parameters
  set_bottom_levels(dag);
  set_top_levels(dag);
  set_precedence_levels(dag);
}
