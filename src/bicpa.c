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
 * size of the target cluster, the makespan, the work and the peak resource
 * usage) after its simulation.
 */
Sched_info_t new_sched_info(int nworkstations, double makespan, double work,
                            int peak_allocation) {
  Sched_info_t s = (Sched_info_t) calloc (1, sizeof(struct _SchedInfo));
  s->nworkstations = nworkstations;
  s->makespan = makespan;
  s->work = work;
  s->peak_allocation = peak_allocation;
  return s;
}

/*
 * Run the simulation of a given 'dag' on a cluster that comprises
 * 'nworkstations' workstations, build and return the data structure with the
 * results of the schedule's simulation: makespan, work and peak resource usage.
 */
Sched_info_t simulate_schedule(xbt_dynar_t dag, int nworkstations){
  double makespan;
  int peak_allocation;
  xbt_dynar_t executed_tasks;
  Sched_info_t s;

  makespan = SD_get_clock ();
  executed_tasks = SD_simulate(-1.);
  xbt_dynar_free_container(&executed_tasks);
  makespan = SD_get_clock () - makespan;

  peak_allocation = compute_peak_resource_usage();
  if (with_communications){
    s = new_sched_info(nworkstations, makespan, makespan * peak_allocation,
        peak_allocation);
  } else {
    s = new_sched_info(nworkstations, makespan, compute_total_work(dag),
        peak_allocation);
  }
  return s;
}

/*
 * Just display the information related to the simulation of a schedule. This
 * display is only available for certain verbosity levels.
 */
void print_sched_info(Sched_info_t s){
  XBT_VERB("[%d] makespan = %.3f, work = %.3f, peak_alloc = %d",
      s->nworkstations, s->makespan, s->work, s->peak_allocation);
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

/*
 * Return the index of the schedule, in a list sorted by increasing makespan
 * values, that minimizes the makespan without degrading the work achieved by
 * the allocation procedure of the seminal CPA algorithm.
 */
int get_best_makespan_index(int nworkstations, Sched_info_t * list,
    double cpa_work){
  int i=0;

  qsort(list, nworkstations, sizeof(Sched_info_t), ImakespanCompareSchedInfo);
  while (list[i]->work > cpa_work)
    i++;
  return i;
}

/*
 * Return the index of the schedule, in a list sorted by increasing work values,
 * that minimizes the work without degrading the makespan achieved by the
 * allocation procedure of the seminal CPA algorithm.
 */
int get_best_work_index(int nworkstations, Sched_info_t *list,
    double cpa_makespan){
  int i=0;

  qsort(list, nworkstations, sizeof(Sched_info_t), IworkCompareSchedInfo);
  while (list[i]->makespan > cpa_makespan)
    i++;
  return i;
}

/*
 * Build a list of non-dominated schedules from all the available schedules. A
 * non-dominated schedule is part of the Pareto front formed by the makespan and
 * work achieved by the different schedules.
 * This list of non-dominated schedules is built by:
 *   - sorting the list of schedule results by increasing makespan values
 *   - browsing this sorted list while considering the work values. If the work
 *     achieved by the current schedule is less or equal than that of the last
 *     non-dominated schedule, then it is also non-dominated. Indeed, there may
 *     a loss in terms of makespan but there is also a gain in terms of work.
 *     Otherwise, there is a loss on both metrics, and the schedule is then
 *     considered as dominated.
 */
void get_non_dominated_schedules(int *nno_dom, Sched_info_t **no_dom_list,
    Sched_info_t *list){
  int i;
  const int nworkstations = SD_workstation_get_number();

  qsort(list, nworkstations, sizeof(Sched_info_t), ImakespanCompareSchedInfo);

  (*no_dom_list) = (Sched_info_t*) calloc (1, sizeof(Sched_info_t));
  (*no_dom_list)[0] = list[0];
  (*nno_dom)++;

  for (i = 1; i < nworkstations; i++){
    if (list[i]->work <= (*no_dom_list)[(*nno_dom)-1]->work){
      (*no_dom_list) = (Sched_info_t*) realloc ((*no_dom_list),
          ((*nno_dom)+1)*sizeof(Sched_info_t));
      (*no_dom_list)[(*nno_dom)++] = list[i];
    }
  }
}

/*
 * Given a list of 'non_dominated_schedules', the makespan and work of the
 * seminal CPA algorithm and a bi-criteria tradeoff to find (either perfect
 * equity or the minimization of the sum, as set by the 'e_or_s' parameter),
 * this function determines the number of workstations that leads to the desired
 * tradeoff.
 */
int get_best_tradeoff_nworkstations(int nschedules, 
    Sched_info_t *non_dominated_schedules,
    double cpa_makespan, double cpa_work, int e_or_s){
  int i;
  int bicriteria_nworkstations = non_dominated_schedules[0]->nworkstations;
  double min, current;

  /*
   * Start by setting the minimum value of the tradeoff using that of the first
   * schedule. For the perfect equity, this corresponds to the ratio of the work
   * over the makespan, normalized by the values achieved by CPA. For the
   * minimization of the sum, these two normalized values are added.
   */
  if (e_or_s)
    min = fabs(1 - ((non_dominated_schedules[0]->work / cpa_work) /
        (non_dominated_schedules[0]->makespan / cpa_makespan)));
  else
    min = (non_dominated_schedules[0]->work / cpa_work) +
    (non_dominated_schedules[0]->makespan / cpa_makespan);


  XBT_DEBUG("[%d]: tradeoff = %f (norm. makespan = %.3f, norm. work = %.3f)",
      non_dominated_schedules[0]->nworkstations, min,
      non_dominated_schedules[0]->makespan / cpa_makespan,
      non_dominated_schedules[0]->work / cpa_work);

  /*
   * Then browse the other non-dominated schedules. If one of them leads to a
   * smaller value for the tradeoff, update the minimum value.
   */
  for (i = 1; i < nschedules; i++){
    if (e_or_s)
      current = fabs(1 - ((non_dominated_schedules[i]->work / cpa_work) /
          (non_dominated_schedules[i]->makespan / cpa_makespan)));
    else
      current = (non_dominated_schedules[i]->work / cpa_work) +
      (non_dominated_schedules[i]->makespan / cpa_makespan);

    XBT_DEBUG("[%d]: tradeoff = %f (norm. makespan = %.3f, norm. work = %.3f)",
        non_dominated_schedules[i]->nworkstations, current,
        non_dominated_schedules[i]->makespan / cpa_makespan,
        non_dominated_schedules[i]->work / cpa_work);

    if (current < min){
      min = current;
      bicriteria_nworkstations = non_dominated_schedules[i]->nworkstations;
    }
  }

  /*
   * Return the number of workstations that leads to the minimum value for the
   * desired bi-criteria tradeoff.
   */
  XBT_VERB("Best tradeoff (%.3f) is achived with %d workstations in the "
      "cluster", min, bicriteria_nworkstations);
  return bicriteria_nworkstations;
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
   * Loop to dynamically change the assumed size of the target cluster from 1
   * to the total number of workstations ('nworkstations'). This impacts the
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
         * Estimate what would be the 'current_gain' in terms of reduction of
         * the execution time if the task is allocated on one more workstation.
         * If the task is already allocated on the full cluster, this
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


void schedule_with_biCPA(xbt_dynar_t dag) {
  unsigned int i, j;
  int best_makespan_nworkstations, best_work_nworkstations;
  int perfect_equity_nworkstations, min_sum_nworkstations;
  const int nworkstations = SD_workstation_get_number();
  double cpa_makespan, cpa_work;
  double alloc_time, mapping_time;
  SD_task_t task;
  int nno_dom=0;
  Sched_info_t *siList, *no_dom_list=NULL;

  siList = (Sched_info_t*) calloc (nworkstations, sizeof(Sched_info_t));

  /*
   * First step: Determine multiple allocations for each task, one for each
   * assumed size of the target cluster between 1 and nworkstations.
   */
  alloc_time = getTime();
  set_multiple_allocations (dag);
  alloc_time = getTime() - alloc_time;
  XBT_VERB("Allocations built in %f seconds", alloc_time);

  /* Display all allocations in DEBUG mode */
  if (XBT_LOG_ISENABLED(heuristic, xbt_log_priority_debug))
    xbt_dynar_foreach(dag, i, task)
      if (SD_task_get_kind(task) == SD_TASK_COMP_PAR_AMDAHL){
        XBT_DEBUG("Intermediate allocations of task '%s' are:",
            SD_task_get_name(task));
        for(j = 1; j < nworkstations; j++)
          XBT_DEBUG(" - %d: %d", j, SD_task_get_iterative_allocations(task, j));
      }

  /*
   * Second step: Build and simulate a schedule for each assumed size of the
   * target cluster. Store the performance metrics (makespan, work, peak
   * resource usage) for each of them.
   */
  mapping_time = getTime();

  for (j = 1; j <= nworkstations; j++){
    set_allocations_from_iteration(dag, j);
    map_allocations(dag);
    siList[j-1] = simulate_schedule(dag, j);
    print_sched_info(siList[j-1]);
    reset_simulation (dag);
  }

  /*
   * The last schedule (on the whole cluster that comprises 'nworkstations'
   * corresponds to the one built from the seminal CPA allocation procedure.
   * The achieved makespan and work will be used to determine the bi-criteria
   * optimizations.
   */
  cpa_makespan = siList[nworkstations-1]->makespan;
  cpa_work = siList[nworkstations-1]->work;

  /*
   * Third step: determine the assumed sizes of the target cluster (hence the
   * tasks' allocations) that lead to the four biCPA variants:
   * biCPA-M: smallest makespan without degrading the work achieved by CPA.
   * biCPA-W: smallest work without degrading the makespan achieved by CPA.
   * biCPA-E: bi-criteria optimization that favors the perfect equity of the
   *          gains w.r.t. CPA in terms of makespan and work.
   * biCPA-S: bi-criteria optimization that minimizes the sum of the gains
   *          w.r.t. CPA in terms of makespan and work.
   */
  for (i = 0; i < nworkstations; i++){
    XBT_VERB("%d: %.3f (%.3f) %.3f (%.3f)", siList[i]->nworkstations,
        siList[i]->makespan, siList[i]->makespan/cpa_makespan,
        siList[i]->work, siList[i]->work/cpa_work);
  }

  best_work_nworkstations =
      siList[get_best_work_index(nworkstations,
          siList, cpa_makespan)]->nworkstations;

  best_makespan_nworkstations =
      siList[get_best_makespan_index(nworkstations,
          siList, cpa_work)]->nworkstations;

  get_non_dominated_schedules(&nno_dom, &no_dom_list, siList);
  perfect_equity_nworkstations =
      get_best_tradeoff_nworkstations(nno_dom, no_dom_list,
          cpa_makespan, cpa_work, 1);

  min_sum_nworkstations= get_best_tradeoff_nworkstations(nno_dom, no_dom_list,
      cpa_makespan, cpa_work, 0);

  XBT_VERB("The four variants of biCPA assumes the following cluster sizes:");
  XBT_VERB("  * biCPA-M: %d", best_makespan_nworkstations);
  XBT_VERB("  * biCPA-W: %d", best_work_nworkstations);
  XBT_VERB("  * biCPA-E: %d", perfect_equity_nworkstations);
  XBT_VERB("  * biCPA-S: %d", min_sum_nworkstations);

  mapping_time = getTime() - mapping_time;

  /*
   * Display the output of the function by retrieving the scheduling results
   * from 'siList' for the respective number of workstations of the four
   * variants of the biCPA algorithm. For comparison purposes, the results
   * achieved by the seminal CPA algorithm (i.e., using the whole cluster to
   * determine the tasks' allocations) are also displayed.
   */
  for (i = 0; i < nworkstations; i++){
    if (siList[i]->nworkstations == best_makespan_nworkstations)
      printf("%.3f:%.3f:biCPA-M:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
          platform_file, dagfile,
          siList[i]->makespan,
          siList[i]->work,
          siList[i]->peak_allocation);
    if (siList[i]->nworkstations == best_work_nworkstations)
      printf("%.3f:%.3f:biCPA-W:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
          platform_file, dagfile,
          siList[i]->makespan,
          siList[i]->work,
          siList[i]->peak_allocation);
    if (siList[i]->nworkstations == perfect_equity_nworkstations)
      printf("%.3f:%.3f:biCPA-E:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
          platform_file, dagfile,
          siList[i]->makespan,
          siList[i]->work,
          siList[i]->peak_allocation);
    if (siList[i]->nworkstations == min_sum_nworkstations)
      printf("%.3f:%.3f:biCPA-S:%s:%s:%.3f:%.3f:%d\n", alloc_time, mapping_time,
          platform_file, dagfile,
          siList[i]->makespan,
          siList[i]->work,
          siList[i]->peak_allocation);
    if (siList[i]->nworkstations == nworkstations)
      printf("*****:*****:  CPA  :%s:%s:%.3f:%.3f:%d\n",
          platform_file, dagfile,
          siList[i]->makespan,
          siList[i]->work,
          siList[i]->peak_allocation);
  }

  free(no_dom_list);
  for (i=0; i < nworkstations; i++)
    free(siList[i]);
  free(siList);
}
