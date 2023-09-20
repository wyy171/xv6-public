#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
    int ctime, etime;

    int pid = fork();
    if (pid == 0) {
        exec("uniq", argv);
    } else {
        waitx(&ctime, &etime);   
        printf(1, "Process uniq: ctime=%d, etime=%d, rtime=%d\n", ctime, etime, etime-ctime);
   
    }

    pid = fork();
    if (pid == 0) {
        exec("head", argv);
    } else {
        waitx(&ctime, &etime);   
        printf(1, "Process head: ctime=%d, etime=%d, rtime=%d\n", ctime, etime, etime-ctime);
    }

    exit();
}
