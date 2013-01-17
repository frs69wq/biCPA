#include "simdag/simdag.h"
#include "xbt.h"
#include "task.h"
#include "workstation.h"
#include "simdag/simdag.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(task, biCPA, "Logging specific to tasks");

void SD_task_allocate_attribute(SD_task_t task){
  TaskAttribute attr = calloc(1,sizeof(struct _TaskAttribute));
  attr->marked = 0;
  SD_task_set_data(task, attr);
}

TaskAttribute new_task_attribute(){
  TaskAttribute attr = (TaskAttribute) calloc (1,
                                               sizeof(struct _TaskAttribute));
  attr->marked = 0;
  return attr;
}

void SD_task_free_attribute(SD_task_t task){
  free(SD_task_get_data(task));
  SD_task_set_data(task, NULL);
}

double SD_task_get_mean_cost( SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->mean_cost;
}

void SD_task_set_mean_cost(SD_task_t task, double mean_cost){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->mean_cost = mean_cost;
  SD_task_set_data(task, attr);
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

int SD_task_get_nworkstations(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->nworkstations;
}
void SD_task_set_nworkstations(SD_task_t task, int nworkstations){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  attr->nworkstations = nworkstations;
  SD_task_set_data(task, attr);
}

int SD_task_is_marked(SD_task_t task){
  TaskAttribute attr = (TaskAttribute) SD_task_get_data(task);
  return attr->marked;
}

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

double bottom_level_recursive_computation(SD_task_t task){
  unsigned int i;
  double my_bottom_level = 0.0, max_bottom_level,
      current_child_bottom_level = 0.0;
  SD_task_t child, grand_child;
  xbt_dynar_t children, grand_children;

  my_bottom_level = SD_task_get_mean_cost(task);

  max_bottom_level = -1.0;
  if (!strcmp(SD_task_get_name(task),"end")){
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
        current_child_bottom_level = SD_task_get_bottom_level(grand_child) +
                                     SD_task_get_mean_cost(child);
      } else {
        current_child_bottom_level =
            bottom_level_recursive_computation(grand_child) +
            SD_task_get_mean_cost(child);
      }
      xbt_dynar_free_container(&grand_children);
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
                                   SD_task_get_mean_cost(grand_parent) +
                                   SD_task_get_mean_cost(parent);
      } else {
        current_parent_top_level =
            top_level_recursive_computation(grand_parent) +
            SD_task_get_mean_cost(grand_parent)+
            SD_task_get_mean_cost(parent);
      }
    }

    if (max_top_level < current_parent_top_level)
      max_top_level = current_parent_top_level;

    xbt_dynar_free_container(&grand_parents);
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
    }

    if (my_prec_level < current_parent_prec_level)
      my_prec_level = current_parent_prec_level;

    xbt_dynar_free_container(&grand_parents);
  }

  SD_task_set_precedence_level(task, my_prec_level);
  SD_task_mark(task);
  XBT_DEBUG("%s's precedence level is %d", SD_task_get_name(task),
      my_prec_level);

  xbt_dynar_free_container(&parents);

  return my_prec_level;
}

// TODO adapt to parallel tasks
void add_resource_dependency(SD_task_t task, SD_workstation_t workstation){
  SD_task_t last_scheduled_task =
      SD_workstation_get_last_scheduled_task(workstation);
  if ((last_scheduled_task) &&
      (SD_task_get_state(last_scheduled_task) != SD_DONE) &&
      (SD_task_get_state(last_scheduled_task) != SD_FAILED) &&
      (!SD_task_dependency_exists(SD_workstation_get_last_scheduled_task(
          workstation), task)))
    SD_task_dependency_add("resource", NULL, last_scheduled_task, task);
    SD_workstation_set_last_scheduled_task(workstation, task);
}