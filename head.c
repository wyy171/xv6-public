#include "types.h"
#include "stat.h"
#include "user.h"

int
head_n(int input_fd, int output_fd, int lines) {

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
            printf(output_fd, "%s\n", current_line);
            // Increment the count for the lines
            count ++;
        } 
        else break; 
    }

    return 0; // Success
}

int main(int argc, char *argv[]) {
    int lines = 4;
    if (argc == 4)
        lines = atoi(argv[3]);
   
    printf(2, "Head command is getting executed in kernel mode.\n");
    
    // Invoke the head system call
    int ret = head_n(0, 1, lines);

    if (ret < 0) {
        printf(2, "head: syscall failed\n");
    }

    exit();
}
