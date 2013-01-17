#include <math.h>
#include <string.h>
#include "simdag/simdag.h"
#include "xbt.h"

#include "dag.h"
#include "task.h"
#include "timer.h"
#include "workstation.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(biCPA, biCPA, "Logging specific to biCPA");

typedef struct _SchedInfo *SchedInfo;

struct _SchedInfo {
  int nhosts;
  double makespan;
  double work;
};

SchedInfo newSchedInfo(int nhosts, double makespan, double work) {
  SchedInfo s;
  s = (SchedInfo) calloc (1, sizeof(struct _SchedInfo));
  s->nhosts = nhosts;
  s->makespan = makespan;
  s->work = work;

  return s;
}

int ImakespanCompareSchedInfo(const void *n1, const void *n2) {
  double m1, m2;

  m1 = (*((SchedInfo *)n1))->makespan;
  m2 = (*((SchedInfo *)n2))->makespan;

  if (m1 < m2)
    return -1;
  else if (m1 == m2)
    return 0;
  else
    return 1;
}

int IworkCompareSchedInfo(const void *n1, const void *n2)
{
  double e1, e2;

  e1 = (*((SchedInfo *)n1))->work;
  e2 = (*((SchedInfo *)n2))->work;

  if (e1 < e2)
    return -1;
  else if (e1 == e2)
    return 0;
  else
    return 1;
}

void getNonDominated(int *nno_dom, SchedInfo **no_dom_list, SchedInfo *list){
  int i;
  const int n = SD_workstation_get_number();

  (*no_dom_list) = (SchedInfo*) calloc (1, sizeof(SchedInfo));
  (*no_dom_list)[0] = list[0];
  (*nno_dom)++;

  for (i=1; i<n; i++){
    if (list[i]->work<=(*no_dom_list)[(*nno_dom)-1]->work){
      (*no_dom_list) = (SchedInfo*) realloc ((*no_dom_list),
          ((*nno_dom)+1)*sizeof(SchedInfo));
      (*no_dom_list)[(*nno_dom)++] = list[i];
    }
  }
}

int getBestMakespan(int size, SchedInfo *list, double cpa_work){
  int i=0;

  qsort(list, size, sizeof(SchedInfo), ImakespanCompareSchedInfo);
  while (list[i]->work > cpa_work)
    i++;
  return i;
}

int getBestWork(int size, SchedInfo *list, double cpa_makespan){
  int i=0;

  qsort(list, size, sizeof(SchedInfo), IworkCompareSchedInfo);
  while (list[i]->makespan > cpa_makespan)
    i++;
  return i;
}

int getBiCriteriaTradeoff(int size, SchedInfo *list, double min_c,
                          double min_w, int e_or_s){
  int i;
  int bicriteria_nhosts = list[0]->nhosts;
  double min_diff, current_diff;

  if (e_or_s)
    min_diff = fabs(1-((list[0]->work/min_w)/(list[0]->makespan/min_c)));
  else
    min_diff = (list[0]->work/min_w)+(list[0]->makespan/min_c);

  XBT_DEBUG("%d : Diff of normalized values: %f (%f %f)",
      list[0]->nhosts+1, min_diff,
      list[0]->makespan/min_c,list[0]->work/min_w);

  for (i=1;i<size;i++){
    if (e_or_s)
      current_diff = fabs(1-((list[i]->work/min_w)/(list[i]->makespan/min_c)));
    else
      current_diff = (list[i]->work/min_w)+(list[i]->makespan/min_c);

    XBT_DEBUG("%d : Diff of normalized values: %f (%f %f)",
        list[i]->nhosts+1, current_diff,
        list[i]->makespan/min_c,list[i]->work/min_w);

    if (current_diff < min_diff){
      min_diff = current_diff;
      bicriteria_nhosts = list[i]->nhosts;
    }
  }

  XBT_DEBUG("best nhosts for both criteria is %d", bicriteria_nhosts+1);
  return bicriteria_nhosts;
}

double initTA(xbt_dynar_t dag, int total_nhosts) {
  unsigned int i;
  int nb_comp_nodes = 0;
  const SD_workstation_t *workstations = SD_workstation_get_list();
  double ta = 0.0;
  SD_task_t task;

  xbt_dynar_foreach(dag, i, task){
    if(SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
      nb_comp_nodes++;
      SD_task_set_allocation_size(task,1);
      //TODO have to build the array of computation
      ta += SD_task_get_execution_time(task, 1, workstations,
          SD_task_get_amount(task), 0.0);
    }
  }

  return ta / total_nhosts;
}

void setMultipleAllocations(xbt_dynar_t dag) {
  unsigned int i;
  int n, saturation = 0;
  double delta, delta_tmp, TCP, TA;
  int current_nworkstations = 1;
  int iteration = 0;
  const int nworkstations = SD_workstation_get_number();
  const SD_workstation_t *workstations = SD_workstation_get_list();

  xbt_dynar_t children, grand_children;
  SD_task_t task, child, current_child, selected_task, max_BL_child=NULL;

  TA = initTA(dag,current_nworkstations);
  TCP = SD_task_get_bottom_level(get_dag_root(dag));

  XBT_DEBUG("TA = %.2f, TCP = %.2f", TA, TCP);
  while (current_nworkstations <= nworkstations) {
    if (current_nworkstations > 1)
       TA /= current_nworkstations;

    XBT_DEBUG("current = %d,  TA = %.2f, TCP = %.2f",
        current_nworkstations, TA, TCP);

    while ((TCP > TA) && !saturation) {
      XBT_DEBUG("iteration = %d TA = %.2f TA' = %.2f, TCP = %.2f ",
          iteration,
          (TA*current_nworkstations)/nworkstations,
          TA, TCP);

      selected_task = NULL;
      delta = -1.0;
      task = get_dag_root(dag);

      while (!(strcmp(SD_task_get_name(task), "end"))) {
        children = SD_task_get_children(task);
        xbt_dynar_foreach(children, i, child){
          if (SD_task_get_kind(child) == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
            grand_children = SD_task_get_children(child);
            if (xbt_dynar_length(grand_children) > 1) {
              XBT_WARN("Transfer %s (type = %d) has 2 children",
                       SD_task_get_name(child), SD_task_get_kind(child));
            }
            xbt_dynar_get_cpy(grand_children, 0, &current_child);
          } else {
            current_child = child;
          }
          if (!i ||
              (SD_task_get_bottom_level(max_BL_child) <
                  SD_task_get_bottom_level(current_child))){
            max_BL_child = current_child;
          }
          XBT_DEBUG("Child with biggest bottom level is %s",
              SD_task_get_name(max_BL_child));

          n = SD_task_get_allocation_size(max_BL_child);
          XBT_DEBUG("Current allocation for %s is %d workstations",
              SD_task_get_name(max_BL_child), n);
        }
        if (n < nworkstations){
          //TODO Build the computation arrays
          delta_tmp = SD_task_get_execution_time(max_BL_child, n,
              workstations, SD_task_get_amount(max_BL_child), 0.0) / n -
              SD_task_get_execution_time(max_BL_child, n +1,
                  workstations, SD_task_get_amount(max_BL_child), 0.0) /
                  (n+1);
        } else {
          delta_tmp = 0.0;
        }

        if (delta_tmp > 0.0 && delta < delta_tmp) {
          delta = delta_tmp;
          selected_task = max_BL_child;
        }

        task = max_BL_child;
      }

      if (!selected_task) {	/* The cluster is saturated */
        saturation = 1;
      } else {
        SD_task_set_allocation_size(selected_task,
            SD_task_get_allocation_size(selected_task)+1);
        /* Recompute TCP and TA */
        //TODO re-implement the getArea function
        TA = TA +((getArea(selected_task,
                          SD_task_get_allocation_size(selected_task)) -
                  getArea(selected_task,
                          SD_task_get_allocation_size(selected_task) - 1)) /
                  current_nworkstations);

        set_bottom_levels(dag);
        TCP = SD_task_get_bottom_level(get_dag_root(dag));
      }
      iteration++;
    }

    xbt_dynar_foreach(dag, i, task){
      if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
        //TODO implement this
        SD_task_set_iterative_nworkstations(task, current_nworkstations,
            SD_task_get_allocation_size(task));
      }
    }
    XBT_DEBUG("current = %d,  TA' = %.2f, TCP = %.2f",
        current_nworkstations, TA, TCP);

    current_nworkstations++;

    TA*=current_nworkstations;
  }
}


void bicpaSchedule(xbt_dynar_t dag) {
  unsigned int i;
  int j, tmp;
  int makespan_nhosts, work_nhosts, bicriteria_nhosts;
  const int nworkstations = SD_workstation_get_number();
  int peak_alloc;
  double makespan, total_work;
  double cpa_makespan, cpa_work;
  double alloc_time, mapping_time;
  SD_task_t task;

  int nno_dom=0;
  SchedInfo *siList, *no_dom_list=NULL;

  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task)== SD_TASK_COMP_PAR_AMDAHL){
      SD_task_allocate_iterative_nworkstations(task, nworkstations);//TODO implement this
    }
  }

  siList = (SchedInfo*) calloc (nworkstations, sizeof(SchedInfo));

  alloc_time = getTime();

  setMultipleAllocations (dag);

  alloc_time = getTime() - alloc_time;

  mapping_time = getTime();

  for (j=1; j <= nworkstations; j++){
    // TODO xbt_dynar_sort(dag,bottomLevelCompareTasks);
    xbt_dynar_foreach(dag, i, task){
      if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
        SD_task_set_allocation_size(task,
            SD_task_get_iterative_nworkstations(task, j));//TODO implement this
        SD_task_schedulel(task, SD_task_get_allocation_size(task),
            SD_task_get_workstation_list(task));
        //TODO add resource dependencies
      }
    }
    makespan = SD_get_clock ();
    SD_simulate(-1.);
    makespan = SD_get_clock () - makespan;

    peak_alloc = compute_peak_resource_usage();
    if (with_communications){
      siList[j] = newSchedInfo(j, makespan, makespan * peak_alloc);
    } else {
      siList[j] = newSchedInfo(j, makespan, compute_total_work(dag));
    }

    XBT_DEBUG("[%d] mkspn = %.10f, efficiency = %.10f, peak_alloc = %d",
        siList[j]->nhosts+1, siList[j]->makespan,
        siList[j]->work, peak_alloc);
    reset_simulation (dag);
  }

  cpa_makespan = siList[nworkstations-1]->makespan;
  cpa_work = siList[nworkstations-1]->work;

  tmp = getBestWork(nworkstations, siList, cpa_makespan);

  work_nhosts = siList[tmp]->nhosts;
  total_work = siList[tmp]->work;

  qsort(siList, nworkstations, sizeof(SchedInfo), ImakespanCompareSchedInfo);

  makespan_nhosts =  siList[0]->nhosts;
  makespan = siList[0]->makespan;

  for (i=0 ; i < nworkstations; i++){
    XBT_DEBUG("%.3f %.3f %d", siList[i]->makespan/cpa_makespan,
        siList[i]->work/cpa_work, siList[i]->nhosts+1);
  }

  getNonDominated(&nno_dom, &no_dom_list, siList);
  XBT_DEBUG("And the non dominated are");

  for (i=0;i< nno_dom;i++){
    XBT_DEBUG("%.5f %.5f %d", no_dom_list[i]->makespan/cpa_makespan,
        no_dom_list[i]->work/cpa_work, no_dom_list[i]->nhosts+1);
  }


  bicriteria_nhosts = getBiCriteriaTradeoff(nno_dom, no_dom_list,
      cpa_makespan, cpa_work, 1); //then perfect-equity

  XBT_DEBUG("Best #hosts are: mkspn = %d, eff = %d, bicrit = %d",
      makespan_nhosts+1,
      work_nhosts+1,
      bicriteria_nhosts+1);

  // TODO xbt_dynar_sort(dag,bottomLevelCompareTasks);
  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
      SD_task_set_allocation_size(task,
          SD_task_get_iterative_nworkstations(task, bicriteria_nhosts));
      SD_task_schedulel(task, SD_task_get_allocation_size(task),
          SD_task_get_workstation_list(task));
      //TODO add resource dependencies
    }
  }

  mapping_time = getTime() - mapping_time;

  makespan = SD_get_clock ();
  SD_simulate(-1.);
  makespan = SD_get_clock () - makespan;
  peak_alloc = compute_peak_resource_usage();

  if (with_communications)
    total_work = makespan * peak_alloc;
  else
    total_work = compute_total_work (dag);

  printf("%f:%f:biCPA-E:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
      platform_file, dagfile,
      makespan, total_work, peak_alloc);

  reset_simulation (dag);

  // TODO xbt_dynar_sort(dag,bottomLevelCompareTasks);
  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
      SD_task_set_allocation_size(task,
          SD_task_get_iterative_nworkstations(task, makespan_nhosts));
      SD_task_schedulel(task, SD_task_get_allocation_size(task),
          SD_task_get_workstation_list(task));
      //TODO add resource dependencies
    }
  }

  makespan = SD_get_clock ();
  SD_simulate(-1.);
  makespan = SD_get_clock () - makespan;
  peak_alloc = compute_peak_resource_usage();

  if (with_communications)
    total_work = makespan * peak_alloc;
  else
    total_work = compute_total_work (dag);

  printf("%f:%f:biCPA-M:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
      platform_file, dagfile,
      makespan, total_work, peak_alloc);

  reset_simulation (dag);

  // TODO xbt_dynar_sort(dag,bottomLevelCompareTasks);
  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
      SD_task_set_allocation_size(task,
          SD_task_get_iterative_nworkstations(task, work_nhosts));
      SD_task_schedulel(task, SD_task_get_allocation_size(task),
          SD_task_get_workstation_list(task));
      //TODO add resource dependencies
    }
  }

  makespan = SD_get_clock ();
  SD_simulate(-1.);
  makespan = SD_get_clock () - makespan;
  peak_alloc = compute_peak_resource_usage();

  if (with_communications)
    total_work = makespan * peak_alloc;
  else
    total_work = compute_total_work (dag);

  printf("%f:%f:biCPA-W:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
      platform_file, dagfile,
      makespan, total_work, peak_alloc);

  reset_simulation (dag);

  bicriteria_nhosts= getBiCriteriaTradeoff(nno_dom, no_dom_list,
      cpa_makespan, cpa_work, 0); // then minimizes the sum

  // TODO xbt_dynar_sort(dag,bottomLevelCompareTasks);
  xbt_dynar_foreach(dag, i, task){
    if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
      SD_task_set_allocation_size(task,
          SD_task_get_iterative_nworkstations(task, work_nhosts));
      SD_task_schedulel(task, SD_task_get_allocation_size(task),
          SD_task_get_workstation_list(task));
      //TODO add resource dependencies
    }
  }

  makespan = SD_get_clock ();
  SD_simulate(-1.);
  makespan = SD_get_clock () - makespan;
  peak_alloc = compute_peak_resource_usage();

  if (with_communications)
    total_work = makespan * peak_alloc;
  else
    total_work = compute_total_work (dag);

  printf("%f:%f:biCPA-S:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
      platform_file, dagfile,
      makespan, total_work, peak_alloc);
}
