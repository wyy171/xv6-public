#include "types.h"
#include "user.h"

int main() {
    #struct proc_stat pstat;

    int pid = fork();
    if (pid == 0) {
        exec("uniq", argv);
    } else {
        #wait(&pstat);
        wait();
        printf(1, "Process uniq: ctime=%d, etime=%d, rtime=%d\n", pstat.ctime, pstat.etime, pstat.rtime);
    }

    pid = fork();
    if (pid == 0) {
        exec("head", argv);
    } else {
        wait(&pstat);
        printf(1, "Process head: ctime=%d, etime=%d, rtime=%d\n", pstat.ctime, pstat.etime, pstat.rtime);
    }

    exit();
}
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(argc < 2) {
    printf(2, "Usage: waitx command\n");
    exit();
  }

  int pid = fork();
  if(pid == -1) printf(2, "Fork Error\n");
  else if(pid == 0)
  {
    if(exec(argv[1], argv + 1) == -1) printf(2, "No such command\n");
    exit();
  }
  else
  {
    int rtime, wtime;
    waitx(&rtime, &wtime);
    printf(1, "Running Time : %d\nWaiting Time : %d\n", rtime, wtime);
  }

  exit();
}
