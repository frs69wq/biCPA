#ifndef WORKSTATION_H_
#define WORKSTATION_H_
#include "simdag/simdag.h"

typedef struct _WorkstationAttribute *WorkstationAttribute;
struct _WorkstationAttribute {
  /* Earliest time at wich a workstation is ready to execute a task*/
  double available_at;
  SD_task_t last_scheduled_task;
};

/*
 * Creator and destructor
 */
void SD_workstation_allocate_attribute(SD_workstation_t );
void SD_workstation_free_attribute(SD_workstation_t );

/*
 * Accessors
 */
double SD_workstation_get_available_at(SD_workstation_t);
void SD_workstation_set_available_at(SD_workstation_t, double);
SD_task_t SD_workstation_get_last_scheduled_task( SD_workstation_t workstation);
void SD_workstation_set_last_scheduled_task(SD_workstation_t workstation,
                                            SD_task_t task);

/*
 * Comparators
 */
int nameCompareWorkstations(const void *, const void *);

int getPeakAlloc();
void resetWorkstationAttributes();

extern char* platform_file;

#endif /* WORKSTATION_H_ */
