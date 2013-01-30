/******************************************************************************
 * Copyright (c) 2010-2013. F. Suter
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL 2.1) which comes with this package.
 *****************************************************************************/
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
