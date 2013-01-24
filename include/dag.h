#ifndef DAG_H_
#define DAG_H_

SD_task_t get_dag_root(xbt_dynar_t dag);
SD_task_t get_dag_end(xbt_dynar_t dag);

void set_bottom_levels (xbt_dynar_t dag);
void set_top_levels (xbt_dynar_t dag);
void set_precedence_levels (xbt_dynar_t dag);

double compute_total_work (xbt_dynar_t dag);

void set_allocations_from_iteration(xbt_dynar_t dag, int index);
void map_allocations(xbt_dynar_t dag);
void reset_simulation (xbt_dynar_t dag);

extern int with_communications;
extern char* dagfile;

char* basename (char*);

#endif /* DAG_H_ */
