/******************************************************************************
 * Copyright (c) 2010-2013. F. Suter
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL 2.1) which comes with this package.
 *****************************************************************************/
#ifndef TASK_H_
#define TASK_H_
#include "simdag/simdag.h"

typedef struct _TaskAttribute *TaskAttribute;
struct _TaskAttribute {
  double bottom_level;
  double top_level;
  int precedence_level;

  /* The workstations onto which the node has been scheduled */
  int allocation_size;
  SD_workstation_t *allocation;

  int *iterative_allocations;

  double estimated_finish_time;

  int marked;
};

/*****************************************************************************/
/*****************************************************************************/
/**************          Attribute management functions         **************/
/*****************************************************************************/
/*****************************************************************************/
void SD_task_allocate_attribute(SD_task_t);
void SD_task_free_attribute(SD_task_t);

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

/*****************************************************************************/
/*****************************************************************************/
/**************               Comparison functions              **************/
/*****************************************************************************/
/*****************************************************************************/
int bottomLevelCompareTasks(const void *, const void *);

/*****************************************************************************/
/*****************************************************************************/
/**************                   DFS helpers                   **************/
/*****************************************************************************/
/*****************************************************************************/
void SD_task_mark(SD_task_t task);
int SD_task_is_marked(SD_task_t task);
void SD_task_unmark(SD_task_t task);

/*****************************************************************************/
/*****************************************************************************/
/**************               Estimation functions              **************/
/*****************************************************************************/
/*****************************************************************************/
double SD_task_estimate_execution_time(SD_task_t task, int nworkstations);
double SD_task_estimate_area(SD_task_t task, int nworkstations);
double SD_task_estimate_minimal_start_time(SD_task_t task);
double SD_task_estimate_transfer_time_from(SD_task_t src, SD_task_t dst,
    double size);
double SD_task_estimate_last_data_arrival_time (SD_task_t task);

/*****************************************************************************/
/*****************************************************************************/
/**************              DFS internal functions             **************/
/*****************************************************************************/
/*****************************************************************************/
double bottom_level_recursive_computation(SD_task_t task);
double top_level_recursive_computation(SD_task_t task);
int precedence_level_recursive_computation(SD_task_t task);


#endif /* TASK_H_ */
