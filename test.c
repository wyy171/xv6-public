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
