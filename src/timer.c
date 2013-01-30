/******************************************************************************
 * Copyright (c) 2010-2013. F. Suter
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL 2.1) which comes with this package.
 *****************************************************************************/
#include "timer.h"
#include <stdlib.h>

/*
 * Just a wrapper on gettimeofday to get time in the same format as SimGrid
 */
double get_time(){
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double) (t.tv_sec + t.tv_usec /1e6);
}

