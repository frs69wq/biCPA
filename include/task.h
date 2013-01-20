#ifndef TASK_H_
#define TASK_H_
#include "simdag/simdag.h"

typedef struct _TaskAttribute *TaskAttribute;
struct _TaskAttribute {
  double bottom_level;
  double top_level;
  int precedence_level;

  double estimated_finish_time;
  /* The workstations onto which the node has been scheduled */
  int allocation_size;
  SD_workstation_t *allocation;

  //TODO have to be properly allocated and freed
  int *iterative_allocations;

  int marked;
};

/*
 * Creator and destructor
 */
void SD_task_allocate_attribute(SD_task_t);
void SD_task_free_attribute(SD_task_t);
/*
 * Comparators
 */
int bottomLevelCompareTasks(const void *, const void *);

/*
 * Accessors
 */
double SD_task_get_bottom_level(SD_task_t task);
void SD_task_set_bottom_level(SD_task_t task, double bottom_level);

double SD_task_get_top_level(SD_task_t task);
void SD_task_set_top_level(SD_task_t task, double top_level);

int SD_task_get_precedence_level(SD_task_t task);
void SD_task_set_precedence_level(SD_task_t task, int precedence_level);

int SD_task_get_allocation_size(SD_task_t task);
void SD_task_set_allocation_size(SD_task_t task, int nworkstations);

SD_workstation_t *SD_task_get_allocation(SD_task_t task);
void SD_task_set_allocation(SD_task_t task, SD_workstation_t *workstation_list);

int SD_task_get_iterative_allocations (SD_task_t task, int index);
void SD_task_set_iterative_allocations (SD_task_t task, int index, int size);

double SD_task_get_estimated_finish_time(SD_task_t task);
void SD_task_set_estimated_finish_time(SD_task_t task, double finish_time);

/*
 * Marking nodes
 */
void SD_task_mark(SD_task_t task);
int SD_task_is_marked(SD_task_t task);
void SD_task_unmark(SD_task_t task);

double bottom_level_recursive_computation(SD_task_t task);
double top_level_recursive_computation(SD_task_t task);
int precedence_level_recursive_computation(SD_task_t task);

double SD_task_estimate_execution_time(SD_task_t task, int nworkstations);
double SD_task_estimate_area(SD_task_t task, int nworkstations);
double SD_task_estimate_minimal_start_time(SD_task_t task);

#endif /* TASK_H_ */
