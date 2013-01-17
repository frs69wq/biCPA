#include "timer.h"
#include <stdlib.h>

/*
 * Just a wrapper on gettimeofday to get time in the same format as SimGrid
 */
double getTime(){
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double) (t.tv_sec + t.tv_usec /1e6);
}

