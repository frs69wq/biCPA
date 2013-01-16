#ifndef DAG_H_
#define DAG_H_

SD_task_t get_root(xbt_dynar_t dag);
SD_task_t get_end(xbt_dynar_t dag);

void set_bottom_level (xbt_dynar_t dag);
void set_top_level (xbt_dynar_t dag);
void set_precedence_level (xbt_dynar_t dag);

double computeWork (xbt_dynar_t dag);
void resetSimulation (xbt_dynar_t dag);

extern int comm;
extern char* dagfile;

char* basename (char*);

#endif /* DAG_H_ */
