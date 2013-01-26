#include <math.h>
#include <string.h>
#include "simdag/simdag.h"
#include "xbt.h"

#include "bicpa.h"
#include "dag.h"
#include "task.h"
#include "timer.h"
#include "workstation.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(heuristic, biCPA, "Logging specific to biCPA");

/*
 * Create a data structure to store the results of a schedule (namely the
 * makespan and the work) after its simulation.
 */
Sched_info_t new_sched_info(int nworkstations, double makespan, double work) {
  Sched_info_t s = (Sched_info_t) calloc (1, sizeof(struct _SchedInfo));
  s->nworkstations = nworkstations;
  s->makespan = makespan;
  s->work = work;

  return s;
}

/* Comparison function to sort schedule results by increasing makespan values */
int ImakespanCompareSchedInfo(const void *n1, const void *n2) {
  double m1, m2;

  m1 = (*((Sched_info_t *)n1))->makespan;
  m2 = (*((Sched_info_t *)n2))->makespan;

  if (m1 < m2)
    return -1;
  else if (m1 == m2)
    return 0;
  else
    return 1;
}

/* Comparison function to sort schedule results by increasing work values */
int IworkCompareSchedInfo(const void *n1, const void *n2) {
  double e1, e2;

  e1 = (*((Sched_info_t *)n1))->work;
  e2 = (*((Sched_info_t *)n2))->work;

  if (e1 < e2)
    return -1;
  else if (e1 == e2)
    return 0;
  else
    return 1;
}

void get_non_dominated_schedules(int *nno_dom, Sched_info_t **no_dom_list,
    Sched_info_t *list){
  int i;
  const int n = SD_workstation_get_number();

  (*no_dom_list) = (Sched_info_t*) calloc (1, sizeof(Sched_info_t));
  (*no_dom_list)[0] = list[0];
  (*nno_dom)++;

  for (i=1; i<n; i++){
    if (list[i]->work<=(*no_dom_list)[(*nno_dom)-1]->work){
      (*no_dom_list) = (Sched_info_t*) realloc ((*no_dom_list),
          ((*nno_dom)+1)*sizeof(Sched_info_t));
      (*no_dom_list)[(*nno_dom)++] = list[i];
    }
  }
}
/* Return the */
int get_best_makespan(int size, Sched_info_t *list, double cpa_work){
  int i=0;

  qsort(list, size, sizeof(Sched_info_t), ImakespanCompareSchedInfo);
  while (list[i]->work > cpa_work)
    i++;
  return i;
}

int get_best_work(int size, Sched_info_t *list, double cpa_makespan){
  int i=0;

  qsort(list, size, sizeof(Sched_info_t), IworkCompareSchedInfo);
  while (list[i]->makespan > cpa_makespan)
    i++;
  return i;
}

int getBiCriteriaTradeoff(int size, Sched_info_t *list, double min_c,
                          double min_w, int e_or_s){
  int i;
  int bicriteria_nhosts = list[0]->nworkstations;
  double min_diff, current_diff;

  if (e_or_s)
    min_diff = fabs(1-((list[0]->work/min_w)/(list[0]->makespan/min_c)));
  else
    min_diff = (list[0]->work/min_w)+(list[0]->makespan/min_c);

  XBT_DEBUG("%d : Diff of normalized values: %f (%f %f)",
      list[0]->nworkstations+1, min_diff,
      list[0]->makespan/min_c,list[0]->work/min_w);

  for (i=1;i<size;i++){
    if (e_or_s)
      current_diff = fabs(1-((list[i]->work/min_w)/(list[i]->makespan/min_c)));
    else
      current_diff = (list[i]->work/min_w)+(list[i]->makespan/min_c);

    XBT_DEBUG("%d : Diff of normalized values: %f (%f %f)",
        list[i]->nworkstations+1, current_diff,
        list[i]->makespan/min_c,list[i]->work/min_w);

    if (current_diff < min_diff){
      min_diff = current_diff;
      bicriteria_nhosts = list[i]->nworkstations;
    }
  }

  XBT_DEBUG("best nhosts for both criteria is %d", bicriteria_nhosts+1);
  return bicriteria_nhosts;
}

/*
 * Initialize the average area (sum of the execution time of all compute task,
 * divided by the size of the cluster) assuming that the cluster comprises only
 * one workstation. Then it amounts to compute the sum of the estimated
 * execution times on a single workstation.
 */
double initialize_average_area(xbt_dynar_t dag) {
  unsigned int i;
  double ta = 0.0;
  SD_task_t task;

  xbt_dynar_foreach(dag, i, task)
    if(SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL)
      ta += SD_task_estimate_execution_time(task, 1);

  return ta;
}

/*
 * This procedure determines a set of 'nworkstations' (the size of the target
 * cluster) allocations for each task that composes the DAG. It is based on the
 * CPA allocation procedure which aims at finding a tradeoff between the length
 * of the critical path (TCP) and the average area (TA). It works as follows:
 *   - Initialize TA and TCP by allocating 1 workstation to each task
 *   - While TCP > TA
 *       * Select the task that belongs to the critical that benefits the most
 *         of an extra workstation to reduce its execution time
 *       * Increase the allocation size of this task
 *       * Update TCP and TA
 *   - respect an extra stop condition if no task in the critical path can
 *     increase its allocation without exceeding the size of the cluster.
 *
 * BiCPA extends this procedure by adding an outer loop that dynamically changes
 * the way the average area is computed. It starts by assuming the target
 * cluster comprises only one workstation. Then, each time the tradeoff between
 * TA and TCP is reached, the current allocation for each task is stored and the
 * number of workstations in the cluster is incremented. The final iteration of
 * this outer loop corresponds to the original allocation procedure of the CPA
 * algorithm.
 */
void set_multiple_allocations(xbt_dynar_t dag) {
  unsigned int i;
  int n, saturation = 0;
  double maximum_gain, current_gain, TCP, TA;
  int current_nworkstations = 1;
  int iteration = 0;
  const int nworkstations = SD_workstation_get_number();
  xbt_dynar_t children, grand_children;
  SD_task_t task, child, current_child, selected_task, max_BL_child=NULL;

  /*
   * Initialize TA and TCP assuming the cluster comprises only one workstation.
   * Then each task is allocated on a single workstation.
   */
  TA = initialize_average_area(dag);
  TCP = SD_task_get_bottom_level(get_dag_root(dag));
  XBT_VERB("Initial values for TA and TCP are (%.3f, %.3f)", TA, TCP);

  /*
   * Loop to dynamically change the assumed size of the target cluster from 1 to
   * the total number of workstations ('nworkstations'). This impacts the
   * computation of the average area TA.
   */
  while (current_nworkstations <= nworkstations) {
    XBT_DEBUG("Assume the cluster comprises %d workstations",
        current_nworkstations);
    XBT_DEBUG("  Current values for TA and TCP are (%.3f, %.3f)", TA, TCP);

    /*
     * Allocation procedure from the seminal CPA algorithm for a cluster that
     * comprises 'current_nworkstations' workstations. This procedure stops if:
     *   - the expected compromise between the reduction of the critical path
     *     length and the augmentation of the average area is met
     *   - the critical path length cannot be further reduced as all the tasks
     *     that composite are already allocated on the maximal number of
     *     workstations.
     */
    while ((TCP > TA) && !saturation) {
      XBT_DEBUG("[%d] CPA_TA = %.2f BICPA_TA = %.2f, TCP = %.2f ",
          iteration,
          (TA*current_nworkstations)/nworkstations,
          TA, TCP);

      selected_task = NULL;
      maximum_gain = -1.0;
      task = get_dag_root(dag);

      /*
       * Browse the current critical path of the DAG in top-down fashion, by
       * finding the tasks with the biggest level among compute successors of
       * the current task.
       */
      while ((strcmp(SD_task_get_name(task), "end"))) {
        children = SD_task_get_children(task);
        xbt_dynar_foreach(children, i, child){
          if (SD_task_get_kind(child) == SD_TASK_COMM_PAR_MXN_1D_BLOCK) {
            grand_children = SD_task_get_children(child);
            xbt_dynar_get_cpy(grand_children, 0, &current_child);
            xbt_dynar_free_container(&grand_children);
          } else {
            current_child = child;
          }
          if (!i ||
              (SD_task_get_bottom_level(max_BL_child) <
                  SD_task_get_bottom_level(current_child))){
            max_BL_child = current_child;
          }
        }
        xbt_dynar_free_container(&children);

        XBT_DEBUG("Next candidate task on the critical path is task '%s'",
            SD_task_get_name(max_BL_child));

        n = SD_task_get_allocation_size(max_BL_child);
        XBT_DEBUG("Current allocation for task '%s' is %d workstations",
            SD_task_get_name(max_BL_child), n);

        /*
         * Estimate what would be the 'current_gain' in terms of reduction of the
         * execution time if the task is allocated on one more workstation. If
         * the task is already allocated on the full cluster, this
         * 'current_gain' is zero.
         */
        if (n < nworkstations){
          current_gain = SD_task_estimate_execution_time(max_BL_child, n) / n -
              SD_task_estimate_execution_time(max_BL_child, (n+1)) / (n+1);
        } else {
          current_gain = 0.0;
        }

        /*
         * If the current task lead to a better gain that the one currently
         * stored, consider it as the new candidate for allocation increase and
         * store the new 'maximum_gain' value.
         */
        if (current_gain > 0.0 && maximum_gain < current_gain) {
          maximum_gain = current_gain;
          selected_task = max_BL_child;
        }

        /* Continue to browse the critical path */
        task = max_BL_child;
      }

      if (!selected_task) {
        /*
         * The cluster is saturated. No task can be selected anymore for
         * allocation increase. Stop the allocation procedure
         */
        saturation = 1;
      } else {
        /*
         * 'selected_task' is the task belonging to the critical path that
         * benefits the most of an extra workstation. Increment its allocation
         * size.
         */
        SD_task_set_allocation_size(selected_task,
            SD_task_get_allocation_size(selected_task)+1);
        /*
         * Recompute TA by taking the current value, adding the new area of the
         * selected task and removing its previous area.
         */
        TA = TA +((SD_task_estimate_area(selected_task,
                          SD_task_get_allocation_size(selected_task)) -
                  SD_task_estimate_area(selected_task,
                          SD_task_get_allocation_size(selected_task) - 1)) /
                  current_nworkstations);

        /*
         * Recompute TCP, by resetting the bottom levels with the new
         * allocations and using the bottom level value of 'root'.
         */
        set_bottom_levels(dag);
        TCP = SD_task_get_bottom_level(get_dag_root(dag));
      }
      iteration++;
    }

    /*
     * A tradeoff has been found between TCP and TA. Store the current
     * allocations for the tasks.
     */
    xbt_dynar_foreach(dag, i, task){
      if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
        SD_task_set_iterative_allocations(task, current_nworkstations,
            SD_task_get_allocation_size(task));
      }
    }

    /*
     * Update the average area by using the new assumed size of the target
     * cluster.
     */
    TA = (TA*current_nworkstations)/(current_nworkstations+1);

    /* Increase the assumed size of the target cluster. */
    current_nworkstations++;
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
  xbt_dynar_t executed_tasks;

  int nno_dom=0;
  Sched_info_t *siList, *no_dom_list=NULL;

  siList = (Sched_info_t*) calloc (nworkstations, sizeof(Sched_info_t));

  alloc_time = getTime();

  set_multiple_allocations (dag);

  alloc_time = getTime() - alloc_time;

  if (XBT_LOG_ISENABLED(heuristic, xbt_log_priority_debug)){
    xbt_dynar_foreach(dag, i, task){
      if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
        XBT_DEBUG("Intermediate allocations of task '%s' are:",
            SD_task_get_name(task));
        for(j = 1; j < nworkstations; j++){
          XBT_DEBUG("  - %d : %d", j,
              SD_task_get_iterative_allocations(task, j));
        }
      }
    }
  }

  XBT_VERB("Allocations built in %f seconds", alloc_time);

  mapping_time = getTime();

  for (j = 1; j <= nworkstations; j++){
    set_allocations_from_iteration(dag, j);
    map_allocations(dag);

    makespan = SD_get_clock ();
    executed_tasks = SD_simulate(-1.);
    xbt_dynar_free_container(&executed_tasks);
    makespan = SD_get_clock () - makespan;

    peak_alloc = compute_peak_resource_usage();
    if (with_communications){
      siList[j-1] = new_sched_info(j, makespan, makespan * peak_alloc);
    } else {
      siList[j-1] = new_sched_info(j, makespan, compute_total_work(dag));
    }

    XBT_VERB("[%d] makespan = %.3f, work = %.3f, peak_alloc = %d",
        siList[j-1]->nworkstations, siList[j-1]->makespan,
        siList[j-1]->work, peak_alloc);
    reset_simulation (dag);
  }

  cpa_makespan = siList[nworkstations-1]->makespan;
  cpa_work = siList[nworkstations-1]->work;

  tmp = get_best_work(nworkstations, siList, cpa_makespan);

  work_nhosts = siList[tmp]->nworkstations;
  total_work = siList[tmp]->work;

  qsort(siList, nworkstations, sizeof(Sched_info_t), ImakespanCompareSchedInfo);

  makespan_nhosts =  siList[0]->nworkstations;
  makespan = siList[0]->makespan;

  for (i=0 ; i < nworkstations; i++){
    XBT_DEBUG("%.3f %.3f %d", siList[i]->makespan/cpa_makespan,
        siList[i]->work/cpa_work, siList[i]->nworkstations+1);
  }

  get_non_dominated_schedules(&nno_dom, &no_dom_list, siList);
  XBT_DEBUG("And the non dominated are");

  for (i=0;i< nno_dom;i++){
    XBT_DEBUG("%.5f %.5f %d", no_dom_list[i]->makespan/cpa_makespan,
        no_dom_list[i]->work/cpa_work, no_dom_list[i]->nworkstations+1);
  }


  bicriteria_nhosts = getBiCriteriaTradeoff(nno_dom, no_dom_list,
      cpa_makespan, cpa_work, 1); //then perfect-equity

  XBT_DEBUG("Best #hosts are: mkspn = %d, eff = %d, bicrit = %d",
      makespan_nhosts+1,
      work_nhosts+1,
      bicriteria_nhosts+1);

  set_allocations_from_iteration(dag, bicriteria_nhosts);
  map_allocations(dag);

  mapping_time = getTime() - mapping_time;

  makespan = SD_get_clock ();
  executed_tasks = SD_simulate(-1.);
  xbt_dynar_free_container(&executed_tasks);
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

  set_allocations_from_iteration(dag, makespan_nhosts);
  map_allocations(dag);

  makespan = SD_get_clock ();
  executed_tasks = SD_simulate(-1.);
  xbt_dynar_free_container(&executed_tasks);
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

  set_allocations_from_iteration(dag, work_nhosts);
  map_allocations(dag);

  makespan = SD_get_clock ();
  executed_tasks = SD_simulate(-1.);
  xbt_dynar_free_container(&executed_tasks);
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

  set_allocations_from_iteration(dag, bicriteria_nhosts);
  map_allocations(dag);

  makespan = SD_get_clock ();
  executed_tasks = SD_simulate(-1.);
  xbt_dynar_free_container(&executed_tasks);
  makespan = SD_get_clock () - makespan;
  peak_alloc = compute_peak_resource_usage();

  if (with_communications)
    total_work = makespan * peak_alloc;
  else
    total_work = compute_total_work (dag);

  printf("%f:%f:biCPA-S:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
      platform_file, dagfile,
      makespan, total_work, peak_alloc);
  free(no_dom_list);
  for (i=0; i < nworkstations; i++)
    free(siList[i]);
  free(siList);
}
