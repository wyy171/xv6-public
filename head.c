#include "types.h"
#include "stat.h"
#include "user.h"

int
head_n(int input_fd, int lines) {

    int count = 0;

    while (1) {
        char current_line[1024];
        int n = read(input_fd, current_line, sizeof(current_line));
        
        if (n <= 0) {
            break; // End of file or error
        }

        // Null-terminate the line
        current_line[n] = '\0';

        // Implement -d flag logic (skip duplicate lines)
        if (count < n) {
            printf(1, "%s\n", current_line);
            // Increment the count for the lines
            count ++;
        } 
        else break; 
    }

    return 0; // Success
}

int main(int argc, char *argv[]) {
    int fd;
    int lines = 4;
    
    if(argc == 1){
        head_n(0, lines);
    }
    if(argc == 2){
        if((fd = open(argv[1],0)) < 0){
            printf(2, "head: cannot open %s \n", argv[1]);
            exit();
        }
        head_n(fd, lines);
        close(fd);
        exit();
    }
    if(argc == 4){
        lines = atoi(argv[2]);  
        if((fd = open(argv[3],0)) < 0){
            printf(2, "head: cannot open %s \n", argv[2]);
            exit();
          }
        if (strcmp(argv[1], "-n") != 0){
            printf(2, "head: need -n not %s", argv[0]);
            exit();
          }
         head_n(fd, lines);
         close(fd);
    }
    exit();
}
