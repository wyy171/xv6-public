#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(argc < 2) {
    printf(2, "Usage: test command\n");
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
    int ctime, etime;
    waitx(&ctime, &etime);   
    printf(1, "Process %s: ctime=%d, etime=%d, rtime=%d\n", argv[1], ctime, etime, etime-ctime);
  }

  exit();
}

/*int main(int argc, char *argv[]){

    int pid = fork();
    if (pid == 0) {
        exec("uniq", argv);
        exit();
    } else {
        int ctime, etime;
        waitx(&ctime, &etime);   
        printf(1, "Process uniq: ctime=%d, etime=%d, rtime=%d\n", ctime, etime, etime-ctime);
   
    }

    pid = fork();
    if (pid == 0) {
        exec("head", argv);
        exit();
    } else {
        int ctime, etime;
        waitx(&ctime, &etime);   
        printf(1, "Process head: ctime=%d, etime=%d, rtime=%d\n", ctime, etime, etime-ctime);
    }

    exit();
}*/


