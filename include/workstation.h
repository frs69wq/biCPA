/******************************************************************************
 * Copyright (c) 2010-2013. F. Suter
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL 2.1) which comes with this package.
 *****************************************************************************/
#ifndef WORKSTATION_H_
#define WORKSTATION_H_
#include "simdag/simdag.h"

typedef struct _WorkstationAttribute *WorkstationAttribute;
struct _WorkstationAttribute {
  /* Earliest time at which a workstation is ready to execute a task*/
  double available_at;
  /* To keep track of potential resource dependencies */
  SD_task_t last_scheduled_task;
};

/*****************************************************************************/
/*****************************************************************************/
/**************          Attribute management functions         **************/
/*****************************************************************************/
/*****************************************************************************/
void SD_workstation_allocate_attribute(SD_workstation_t );
void SD_workstation_free_attribute(SD_workstation_t );

double SD_workstation_get_available_at(SD_workstation_t);
void SD_workstation_set_available_at(SD_workstation_t, double);
SD_task_t SD_workstation_get_last_scheduled_task( SD_workstation_t workstation);
void SD_workstation_set_last_scheduled_task(SD_workstation_t workstation,
                                            SD_task_t task);
void reset_workstation_attributes();

/*****************************************************************************/
/*****************************************************************************/
/**************               Comparison functions              **************/
/*****************************************************************************/
/*****************************************************************************/
int nameCompareWorkstations(const void *, const void *);
int availableAtCompareWorkstations(const void *n1, const void *n2);
int NavailableAtCompareWorkstations(const void *n1, const void *n2);

/*****************************************************************************/
/*****************************************************************************/
/**************               Accounting functions              **************/
/*****************************************************************************/
/*****************************************************************************/
int compute_peak_resource_usage();



SD_workstation_t * get_best_workstation_set(double time);
double get_best_workstation_set_earliest_availability(int nworkstations,
    SD_workstation_t * workstations);

extern char* platform_file;

#endif /* WORKSTATION_H_ */
