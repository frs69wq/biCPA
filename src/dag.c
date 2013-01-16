#include "simdag/simdag.h"
#include "xbt.h"
#include "task.h"
#include "workstation.h"

SD_task_t get_root(xbt_dynar_t dag){
  SD_task_t task;
  xbt_dynar_get_cpy(dag, 0, &task);
  return task;
}

SD_task_t get_end(xbt_dynar_t dag){
  SD_task_t task;
  xbt_dynar_get_cpy(dag, xbt_dynar_length(dag)-1, &task);
  return task;
}

void set_bottom_level (xbt_dynar_t dag){
  unsigned int i;
  SD_task_t task;

  task = get_root(dag);
  bottom_level_recursive_computation(task);

  xbt_dynar_foreach(dag, i, task){
    SD_task_unmark(task);
  }
}

void set_top_level (xbt_dynar_t dag){
  unsigned int i;
  SD_task_t task;

  task = get_end(dag);
  top_level_recursive_computation(task);

  xbt_dynar_foreach(dag, i, task){
    SD_task_unmark(task);
  }
}

void set_precedence_level (xbt_dynar_t dag){
  unsigned int i;
  SD_task_t task;

  task = get_root(dag);
  precedence_level_recursive_computation(task);

  xbt_dynar_foreach(dag, i, task){
    SD_task_unmark(task);
  }
}

double computeWork (xbt_dynar_t dag){
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

void resetSimulation (xbt_dynar_t dag) {
  resetWorkstationAttributes();
  SD_application_reinit();
  //TODO check how to reset task scheduling parameters
  set_bottom_level(dag);
  set_top_level(dag);
  set_precedence_level(dag);
}
