#include "timer.h"

double getTime(){
	struct timeval t;
	gettimeofday(&t, NULL);
	return (double) (t.tv_sec + t.tv_usec /1e6);
}

