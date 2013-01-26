#ifndef BICPA_H_
#define BICPA_H_

typedef struct _SchedInfo {
  int nworkstations;
  double makespan;
  double work;
} *Sched_info_t;

void bicpaSchedule(xbt_dynar_t dag);


#endif /* BICPA_H_ */
