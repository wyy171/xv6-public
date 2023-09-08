#include "types.h"
#include "stat.h"
#include "user.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    int cflag = 0, iflag = 0, dflag = 0;
    
    // Process command-line arguments
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "-c") == 0) {
            cflag = 1;
        } else if (strcmp(arg, "-i") == 0) {
            iflag = 1;
        } else if (strcmp(arg, "-d") == 0) {
            dflag = 1;
        } else {
            printf(2, "Usage: uniq [-c] [-i] [-d] < inputfile > outputfile\n");
            exit();
        }
    }
    printf("Uniq command is getting executed in kernel mode.\n");
    
    // Invoke the uniq system call
    int ret = sys_uniq(0, 1, cflag, iflag, dflag);

    if (ret < 0) {
        printf(2, "uniq: syscall failed\n");
    }

    exit();
}

