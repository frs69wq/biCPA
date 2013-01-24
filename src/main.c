#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "simdag/simdag.h"
#include "xbt.h"

#include "bicpa.h"
#include "dag.h"
#include "task.h"
#include "workstation.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(biCPA, "Logging specific to biCPA");

char *platform_file = NULL, *dagfile = NULL;
/* By default, there are no explicit communications between tasks. To have
 * actual data transfers on the network, use the --with-communications flag
 */
int with_communications = 0;

int main(int argc, char **argv) {
  int flag, total_nworkstations = 0;
  unsigned int cursor;
  const SD_workstation_t *workstations = NULL;
  SD_task_t task;
  xbt_dynar_t dag;

  SD_init(&argc, argv);

  /* get rid off some logs that are useless */
  xbt_log_control_set("sd_daxparse.thresh:critical");
  xbt_log_control_set("surf_workstation.thresh:critical");
  xbt_log_control_set("root.fmt:[%9.3r]%e[%13c/%7p]%e%m%n");

  while (1){
    static struct option long_options[] = {
        {"platform", 1, 0, 'a'},
        {"dag", 1, 0, 'b'},
        {"with-communications", 0, 0, 'c'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    flag = getopt_long (argc, argv, "",
                        long_options, &option_index);

    /* Detect the end of the options. */
    if (flag == -1)
      break;

    switch (flag) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
        printf (" with arg %s", optarg);
        printf ("\n");
      break;
    case 'a':
      platform_file = optarg;
      SD_create_environment(platform_file);
      total_nworkstations = SD_workstation_get_number();
      workstations = SD_workstation_get_list();

      /* Sort the hosts by name for sake of simplicity */
      qsort((void *)workstations,total_nworkstations, sizeof(SD_workstation_t),
          nameCompareWorkstations);

      for(cursor=0; cursor<total_nworkstations; cursor++){
        SD_workstation_allocate_attribute(workstations[cursor]);
      }
      break;
    case 'b':
      dagfile = optarg;
      dag = SD_PTG_dotload(dagfile);
      xbt_dynar_foreach(dag, cursor, task) {
        SD_task_allocate_attribute(task);
      }

      set_bottom_levels (dag);

      if (XBT_LOG_ISENABLED(biCPA, xbt_log_priority_verbose)){
        xbt_dynar_foreach(dag, cursor, task) {
          if (SD_task_get_kind(task) != SD_TASK_COMM_PAR_MXN_1D_BLOCK)
            XBT_VERB("%s: bl=%f",
                SD_task_get_name(task), SD_task_get_bottom_level(task));
        }
      }
      break;
    case 'c':
      with_communications = 1;
      break;
    default:
      break;
    }
  }

  bicpaSchedule(dag);

  xbt_dynar_foreach(dag, cursor, task) {
    SD_task_free_attribute(task);
    free(SD_task_get_data(task));
    SD_task_destroy(task);
  }
  xbt_dynar_free_container(&dag);

  for(cursor = 0; cursor < total_nworkstations; cursor++)
    SD_workstation_free_attribute(workstations[cursor]);

  SD_exit();

  return 0;
}


