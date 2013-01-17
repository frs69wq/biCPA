#ifndef DAG_H_
#define DAG_H_

SD_task_t get_dag_root(xbt_dynar_t dag);
SD_task_t get_dag_end(xbt_dynar_t dag);

void set_bottom_levels (xbt_dynar_t dag);
void set_top_levels (xbt_dynar_t dag);
void set_precedence_levels (xbt_dynar_t dag);

double computeWork (xbt_dynar_t dag);
void resetSimulation (xbt_dynar_t dag);

extern int with_communications;
extern char* dagfile;

char* basename (char*);

#endif /* DAG_H_ */
