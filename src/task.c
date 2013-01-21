#include "task.h"
#include "workstation.h"
#include <stdlib.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(task, biCPA, "Logging specific to tasks");

/*****************************************************************************/
/*****************************************************************************/
/**************          Attribute management functions         **************/
/*****************************************************************************/
/*****************************************************************************/

void SD_task_allocate_attribute(SD_task_t task){
  const int nworkstations = SD_workstation_get_number();
  TaskAttribute attr = calloc(1,sizeof(struct _TaskAttribute));
  attr->marked = 0;
  attr->allocation_size = 1;
  attr->iterative_allocations = (int*) calloc (nworkstations, sizeof(int));
  SD_task_set_data(task, attr);
}

void SD_task_free_attribute(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  free(attr->allocation);
  free(attr->iterative_allocations);
  free(attr);
  SD_task_set_data(task, NULL);
}

double SD_task_get_bottom_level( SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->bottom_level;
}

void SD_task_set_bottom_level(SD_task_t task, double bottom_level){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->bottom_level = bottom_level;
  SD_task_set_data(task, attr);
}

double SD_task_get_top_level( SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->top_level;
}

void SD_task_set_top_level(SD_task_t task, double top_level){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->top_level = top_level;
  SD_task_set_data(task, attr);
}

int SD_task_get_precedence_level( SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->precedence_level;
}

void SD_task_set_precedence_level(SD_task_t task, int precedence_level){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->precedence_level = precedence_level;
  SD_task_set_data(task, attr);
}

int SD_task_get_allocation_size(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->allocation_size;
}

void SD_task_set_allocation_size(SD_task_t task, int nworkstations){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->allocation_size = nworkstations;
  SD_task_set_data(task, attr);
}

SD_workstation_t *SD_task_get_allocation(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->allocation;
}

void SD_task_set_allocation(SD_task_t task,
    SD_workstation_t *workstation_list){
  int i;
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  if (attr->allocation)
    free(attr->allocation);
  attr->allocation = (SD_workstation_t*) calloc (attr->allocation_size,
      sizeof(SD_workstation_t));
  for (i = 0; i < attr->allocation_size; i++){
    attr->allocation[i] = workstation_list[i];
  }
  SD_task_set_data(task, attr);
}

int SD_task_get_iterative_allocations (SD_task_t task, int index){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->iterative_allocations[index-1];
}

void SD_task_set_iterative_allocations (SD_task_t task, int index, int size){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->iterative_allocations[index-1] = size;
  SD_task_set_data(task, attr);
}

double SD_task_get_estimated_finish_time(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->estimated_finish_time;
}

void SD_task_set_estimated_finish_time(SD_task_t task, double finish_time){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->estimated_finish_time = finish_time;
  SD_task_set_data(task, attr);
}

/*****************************************************************************/
/*****************************************************************************/
/**************               Comparison functions              **************/
/*****************************************************************************/
/*****************************************************************************/

/* List scheduling algorithms usually sort tasks according to an objective
 * function. Here the bottom level value is used. Sorting tasks by non
 * increasing bottom values ensures that all predecessors of a task are
 * scheduled before this taks is considered for scheduling.
 */
int bottomLevelCompareTasks(const void *n1, const void *n2)
{
  double bottom_level1, bottom_level2;

  bottom_level1 = SD_task_get_bottom_level((*((SD_task_t *)n1)));
  bottom_level2 = SD_task_get_bottom_level((*((SD_task_t *)n2)));

  if (bottom_level1 > bottom_level2)
    return -1;
  else if (bottom_level1 == bottom_level2)
    return 0;
  else
    return 1;
}

/*****************************************************************************/
/*****************************************************************************/
/**************                   DFS helpers                   **************/
/*****************************************************************************/
/*****************************************************************************/
/*
 * During the Depth-First Search traversal of the DAG used to compute bottom,
 * top, and precedence levels, it is mandatory to mark task as visited to
 * prevent useless computations. The functions below mark and unmark tasks. A
 * test to know whether a task is marked or not is also available.
 */
void SD_task_mark(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->marked=1;
  SD_task_set_data(task, attr);
}

void SD_task_unmark(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->marked=0;
  SD_task_set_data(task, attr);
}

int SD_task_is_marked(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->marked;
}

/*****************************************************************************/
/*****************************************************************************/
/**************               Estimation functions              **************/
/*****************************************************************************/
/*****************************************************************************/

/*
 * Return a rough estimation of what would be the execution time of task given
 * as input on a given number of workstations. The task has to be of kind
 * SD_TASK_COMP_PAR_AMDAHL, as Amdahl's law is applied to get this estimation.
 * It also assumes a fully homogeneous set of workstations as no distinction is
 * made within the whole set.
 */
double SD_task_estimate_execution_time(SD_task_t task, int nworkstations){
  const SD_workstation_t *workstations = SD_workstation_get_list();
  double amount, alpha, power, estimate;

  amount = SD_task_get_amount(task);
  alpha = SD_task_get_alpha(task);
  power = SD_workstation_get_power(workstations[0]);
  estimate = (alpha + (1 - alpha)/nworkstations) * (amount/power);

  XBT_DEBUG("Estimation for task %s is: %f seconds",
      SD_task_get_name(task), estimate);
  return estimate;
}
/*
 * Return a rough estimation of what would be the area taken by the task given
 * as input, i.e., its execution time multiplied by the number of workstations
 * the task is allocated on. This function has the same restrictions and makes
 * the same assumptions as the SD_task_estimate_execution_time() function.
 */
double SD_task_estimate_area(SD_task_t task, int nworkstations) {
  return SD_task_estimate_execution_time(task, nworkstations) * nworkstations;
}

/*
 * Return an estimation of the minimal time before which a task can start. This
 * time depends on the estimated finished time of the compute ancestors of the
 * task, as set when they have been scheduled. Two cases are considered,
 * depending on whether an ancestor is 'linked' to the task through a flow or
 * control dependency. Flow dependencies imply to look at the grand parents of
 * the task, while control dependencies look at the parent tasks directly.
 */
double SD_task_estimate_minimal_start_time(SD_task_t task){
  unsigned int i;
  double min_start_time=0.0;
  xbt_dynar_t parents, grand_parents;
  SD_task_t parent, grand_parent;

  parents = SD_task_get_parents(task);
  xbt_dynar_foreach(parents, i, parent){
    if (SD_task_get_kind(parent) == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
      grand_parents = SD_task_get_parents(parent);
      xbt_dynar_get_cpy(grand_parents, 0, &grand_parent);
        if (SD_task_get_estimated_finish_time(grand_parent) > min_start_time)
          min_start_time = SD_task_get_estimated_finish_time(grand_parent);
    }
    else{
      if (SD_task_get_estimated_finish_time(parent) > min_start_time)
        min_start_time = SD_task_get_estimated_finish_time(parent);
    }
  }
  return min_start_time;
}

double SD_task_estimate_transfer_time_from(SD_task_t src, SD_task_t dst,
    double size){
  int src_allocation_size, dst_allocation_size;
  SD_workstation_t *src_allocation, *dst_allocation;
  const SD_link_t* route=NULL;
  int route_size;
  double transfer_time =0.0;
  int src_index = -1, dst_index=-1;
  int i, s, d;;

  src_allocation_size = SD_task_get_allocation_size(src);
  src_allocation = SD_task_get_allocation(src);
  dst_allocation_size = SD_task_get_allocation_size(dst);
  dst_allocation = SD_task_get_allocation(dst);

  qsort((void *)src_allocation,src_allocation_size, sizeof(SD_workstation_t),
      nameCompareWorkstations);
  qsort((void *)dst_allocation,dst_allocation_size, sizeof(SD_workstation_t),
      nameCompareWorkstations);

  i = 0;
  if (src_allocation_size == dst_allocation_size) {
    for (s = 0; s < src_allocation_size; s++) {
      for (d = 0; d < dst_allocation_size; d++) {
        if (!strcmp(SD_workstation_get_name (src_allocation[s]),
            SD_workstation_get_name (dst_allocation[d]))) {
          i++;
          break;
        }
      }
      if (i <= s)
        break;
    }
  }

  if (i == src_allocation_size) {
    /* both configurations are identical. Let just consider the transfer between
     * the first workstation of each set */
    route= SD_route_get_list (src_allocation[0], dst_allocation[0]);
    route_size = SD_route_get_size(src_allocation[0], dst_allocation[0]);
    src_index=dst_index=0;
  } else {
    /* Found 2 different hosts. The first host belongs to src
       and the second one belongs to dst. Then
       compute the transfer time walking through the route between
       them */
    for (s = 0; s < src_allocation_size; s++) {
      for (d = 0; d < dst_allocation_size; d++) {
        if (strcmp(SD_workstation_get_name (src_allocation[s]),
            SD_workstation_get_name (dst_allocation[d]))) {
          src_index = s;
          dst_index = d;
          break;
        }
      }
      if (src_index != -1)
        break;
    }
    route= SD_route_get_list (src_allocation[src_index],
        dst_allocation[dst_index]);
    route_size = SD_route_get_size(src_allocation[src_index],
        dst_allocation[dst_index]);
  }

 /* first link */
  transfer_time = size/
      (SD_link_get_current_bandwidth(route[0])*src_allocation_size);

  for (i = 1; i < route_size - 1; i++) {
    if (transfer_time < (size /
        SD_link_get_current_bandwidth(route[i])))
      transfer_time = size / SD_link_get_current_bandwidth(route[i]);
  }

  /* last link */
  if (transfer_time < size /
      (SD_link_get_current_bandwidth(route[route_size-1]) *
          dst_allocation_size)){
    transfer_time = size /
        (SD_link_get_current_bandwidth(route[route_size-1]) *
            dst_allocation_size);
  }

  transfer_time +=
      SD_route_get_current_latency (src_allocation[src_index],
          dst_allocation[dst_index]);
  XBT_VERB("Estimated transfer time between tasks '%s' and '%s': %.3f",
      SD_task_get_name(src), SD_task_get_name(dst),transfer_time);
  return transfer_time;
}


double SD_task_estimate_last_data_arrival_time (SD_task_t task){
  unsigned int i;
  double last_data_arrival = -1., data_availability, estimated_transfer_time;
  SD_task_t parent, grand_parent;
  xbt_dynar_t parents, grand_parents;

  parents = SD_task_get_parents(task);

  xbt_dynar_foreach(parents, i, parent){
    if (SD_task_get_kind(parent) == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
      grand_parents = SD_task_get_parents(parent);
      xbt_dynar_get_cpy(grand_parents, 0, &grand_parent);
      estimated_transfer_time =
          SD_task_estimate_transfer_time_from(grand_parent, task,
              SD_task_get_amount(parent));
      data_availability = SD_task_get_estimated_finish_time(grand_parent)+
          estimated_transfer_time;
    } else {
      data_availability = SD_task_get_estimated_finish_time(parent);
    }

    if (last_data_arrival < data_availability)
      last_data_arrival = data_availability;
  }
  return last_data_arrival;
}

/*****************************************************************************/
/*****************************************************************************/
/**************              DFS internal functions             **************/
/*****************************************************************************/
/*****************************************************************************/

double bottom_level_recursive_computation(SD_task_t task){
  unsigned int i;
  double my_bottom_level = 0.0, max_bottom_level,
      current_child_bottom_level = 0.0;
  SD_task_t child, grand_child;
  xbt_dynar_t children, grand_children;

  my_bottom_level = SD_task_estimate_execution_time(task,
      SD_task_get_allocation_size(task));

  max_bottom_level = -1.0;
  if (!strcmp(SD_task_get_name(task),"end")){
    XBT_DEBUG("end's bottom level is 0.0");
    SD_task_mark(task);
    SD_task_set_bottom_level(task, 0.0);
    return 0.0;
  }

  children = SD_task_get_children(task);

  xbt_dynar_foreach(children, i, child){
    if (SD_task_get_kind(child) == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
      grand_children = SD_task_get_children(child);
      if (xbt_dynar_length(grand_children) > 1) {
        XBT_WARN("Transfer %s (type = %d) has 2 children",
                 SD_task_get_name(child), SD_task_get_kind(child));
      }
      xbt_dynar_get_cpy(grand_children, 0, &grand_child);
      if (SD_task_is_marked(grand_child)){
        current_child_bottom_level = SD_task_get_bottom_level(grand_child);
      } else {
        current_child_bottom_level =
            bottom_level_recursive_computation(grand_child);
      }
      xbt_dynar_free_container(&grand_children);
    } else {
      if (SD_task_is_marked(child)){
        current_child_bottom_level = SD_task_get_bottom_level(child);
      } else {
        current_child_bottom_level =
            bottom_level_recursive_computation(child);
      }
    }

    if (max_bottom_level < current_child_bottom_level)
      max_bottom_level = current_child_bottom_level;
  }

  my_bottom_level += max_bottom_level;

  SD_task_set_bottom_level(task, my_bottom_level);
  SD_task_mark(task);
  XBT_DEBUG("%s's bottom level is %f", SD_task_get_name(task), my_bottom_level);

  xbt_dynar_free_container(&children);
  return my_bottom_level ;
}

double top_level_recursive_computation(SD_task_t task){
  unsigned int i;
  double max_top_level = 0.0, my_top_level, current_parent_top_level = 0.0;
  SD_task_t parent, grand_parent=NULL;
  xbt_dynar_t parents, grand_parents;

  max_top_level = -1.0;

  if (!strcmp(SD_task_get_name(task),"root")){
    XBT_DEBUG("root's top level is 0.0");
    SD_task_mark(task);
    SD_task_set_top_level(task, 0.0);
    return 0.0;
  }

  parents = SD_task_get_parents(task);

  xbt_dynar_foreach(parents, i, parent){
    if (SD_task_get_kind(parent) == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
      grand_parents = SD_task_get_parents(parent);
      if (xbt_dynar_length(grand_parents)>1) {
        XBT_WARN("Transfer %s (type = %d) has 2 parents",
                 SD_task_get_name(parent), SD_task_get_kind(parent));
      }
      xbt_dynar_get_cpy(grand_parents, 0, &grand_parent);
      if (SD_task_is_marked(grand_parent)){
        current_parent_top_level = SD_task_get_top_level(grand_parent) +
            SD_task_estimate_execution_time(grand_parent,
                SD_task_get_allocation_size(grand_parent));
      } else {
        current_parent_top_level =
            top_level_recursive_computation(grand_parent) +
            SD_task_estimate_execution_time(grand_parent,
                SD_task_get_allocation_size(grand_parent));
      }
      xbt_dynar_free_container(&grand_parents);
    } else {
      if (SD_task_is_marked(parent)){
        current_parent_top_level = SD_task_get_top_level(parent) +
            SD_task_estimate_execution_time(parent,
                SD_task_get_allocation_size(parent));
      } else {
        current_parent_top_level =
            top_level_recursive_computation(parent) +
            SD_task_estimate_execution_time(parent,
                SD_task_get_allocation_size(parent));
      }
    }

    if (max_top_level < current_parent_top_level)
      max_top_level = current_parent_top_level;
  }

  my_top_level = max_top_level;

  SD_task_set_top_level(task, my_top_level);
  SD_task_mark(task);
  XBT_DEBUG("%s's top level is %f", SD_task_get_name(task), my_top_level);

  xbt_dynar_free_container(&parents);

  return my_top_level;
}

int precedence_level_recursive_computation(SD_task_t task){
  unsigned int i;
  int my_prec_level = -1, current_parent_prec_level = 0;
  SD_task_t parent, grand_parent=NULL;
  xbt_dynar_t parents, grand_parents;



  if (!strcmp(SD_task_get_name(task),"root")){
    XBT_DEBUG("root's precedence level is 0.0");
    SD_task_mark(task);
    SD_task_set_precedence_level(task, 0);
    return 0;
  }

  parents = SD_task_get_parents(task);

  xbt_dynar_foreach(parents, i, parent){
    if (SD_task_get_kind(parent) == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
      grand_parents = SD_task_get_parents(parent);
      if (xbt_dynar_length(grand_parents)>1) {
        XBT_WARN("Transfer %s (type = %d) has 2 parents",
                 SD_task_get_name(parent), SD_task_get_kind(parent));
      }
      xbt_dynar_get_cpy(grand_parents, 0, &grand_parent);
      if (SD_task_is_marked(grand_parent)){
        current_parent_prec_level =
            SD_task_get_precedence_level(grand_parent) + 1;
      } else {
        current_parent_prec_level =
            precedence_level_recursive_computation(grand_parent) + 1;
      }
      xbt_dynar_free_container(&grand_parents);
    } else {
      if (SD_task_is_marked(parent)){
        current_parent_prec_level =
            SD_task_get_precedence_level(parent) + 1;
      } else {
        current_parent_prec_level =
            precedence_level_recursive_computation(parent) + 1;
      }
    }

    if (my_prec_level < current_parent_prec_level)
      my_prec_level = current_parent_prec_level;
  }

  SD_task_set_precedence_level(task, my_prec_level);
  SD_task_mark(task);
  XBT_DEBUG("%s's precedence level is %d", SD_task_get_name(task),
      my_prec_level);

  xbt_dynar_free_container(&parents);

  return my_prec_level;
}


// TODO adapt to parallel tasks
//void add_resource_dependency(SD_task_t task, SD_workstation_t workstation){
//  SD_task_t last_scheduled_task =
//      SD_workstation_get_last_scheduled_task(workstation);
//  if ((last_scheduled_task) &&
//      (SD_task_get_state(last_scheduled_task) != SD_DONE) &&
//      (SD_task_get_state(last_scheduled_task) != SD_FAILED) &&
//      (!SD_task_dependency_exists(SD_workstation_get_last_scheduled_task(
//          workstation), task)))
//    SD_task_dependency_add("resource", NULL, last_scheduled_task, task);
//    SD_workstation_set_last_scheduled_task(workstation, task);
//}
