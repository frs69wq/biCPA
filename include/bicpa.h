#ifndef BICPA_H_
#define BICPA_H_

typedef struct _SchedInfo {
  int nworkstations;
  double makespan;
  double work;
  int peak_allocation;
} *Sched_info_t;

void schedule_with_biCPA(xbt_dynar_t dag);


#endif /* BICPA_H_ */
