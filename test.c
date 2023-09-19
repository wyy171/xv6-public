#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
    int ctime, etime;

    int pid = fork();
    if (pid == 0) {
        exec("uniq", argv);
    } else {
        wait(&ctime, &etime);   
        printf(1, "Process uniq: ctime=%d, etime=%d, rtime=%d\n", ctime, etime, etime-ctime);
   
    }

    pid = fork();
    if (pid == 0) {
        exec("head", argv);
    } else {
        wait(&ctime, &etime);   
        printf(1, "Process head: ctime=%d, etime=%d, rtime=%d\n", ctime, etime, etime-ctime);
    }

    exit();
}
