#include <stdlib.h>
#include "workstation.h"
#include "simdag/simdag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(workstation, biCPA,
    "Logging specific to workstations");

/*****************************************************************************/
/*****************************************************************************/
/**************          Attribute management functions         **************/
/*****************************************************************************/
/*****************************************************************************/

void SD_workstation_allocate_attribute(SD_workstation_t workstation){
  WorkstationAttribute data;
  data = calloc(1,sizeof(struct _WorkstationAttribute));
  SD_workstation_set_data(workstation, data);
}

void SD_workstation_free_attribute(SD_workstation_t workstation){
  free(SD_workstation_get_data(workstation));
  SD_workstation_set_data(workstation, NULL);
}

double SD_workstation_get_available_at( SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  return attr->available_at;
}

void SD_workstation_set_available_at(SD_workstation_t workstation, double time){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->available_at=time;
  SD_workstation_set_data(workstation, attr);
}

SD_task_t SD_workstation_get_last_scheduled_task( SD_workstation_t workstation){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  return attr->last_scheduled_task;
}

void SD_workstation_set_last_scheduled_task(SD_workstation_t workstation,
                                            SD_task_t task){
  WorkstationAttribute attr =
    (WorkstationAttribute) SD_workstation_get_data(workstation);
  attr->last_scheduled_task=task;
  SD_workstation_set_data(workstation, attr);
}

/*
 * Used to start a new simulation within the same run on the fresh basis.
 * This function browses the list of workstations and set back attributes to
 * their initial values:
 *   - available_at = 0.0
 *   - last_scheduled_task = NULL
 */
void reset_workstation_attributes() {
  //TODO check if 0.0 is a good value. Maybe better to use the current
  //     simulated time
  int i;
  int nworkstations = SD_workstation_get_number();
  const SD_workstation_t *workstations = SD_workstation_get_list();

  for (i = 0; i < nworkstations; i++){
    SD_workstation_set_available_at(workstations[i], 0.0);
    SD_workstation_set_last_scheduled_task(workstations[i], NULL);
  }
}

/*****************************************************************************/
/*****************************************************************************/
/**************               Comparison functions              **************/
/*****************************************************************************/
/*****************************************************************************/

/*
 * Sort workstations by name in the LEXICOGRAPHIC order
 */
int nameCompareWorkstations(const void *w1, const void *w2){
  return strcmp(SD_workstation_get_name(*((SD_workstation_t *)w1)),
      SD_workstation_get_name(*((SD_workstation_t *)w2)));
}

/*****************************************************************************/
/*****************************************************************************/
/**************               Accounting functions              **************/
/*****************************************************************************/
/*****************************************************************************/

/*
 * Determine how many distinct workstations have been used by a given schedule.
 * This function is called once the simulation is over. It simply browses the
 * list of all workstations and check the 'available_at' attribute. If its
 * value is greater than 0.0 this means that at least one has been executed
 * there, thus incrementing the peak value.
 */
int compute_peak_resource_usage() {
  //TODO check if 0.0 is a good value. Maybe better to use the start time of the
  //     current simulation
  int i, peak=0;
  int nworkstations = SD_workstation_get_number();
  const SD_workstation_t *workstations = SD_workstation_get_list();
  for (i = 0; i < nworkstations; i++)
    if (SD_workstation_get_available_at(workstations[i]) > 0.0)
      peak++;

  return peak;
}

