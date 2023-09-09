#include "types.h"
#include "stat.h"
#include "user.h"


void print_head_lines(int input_fd, int n){
    int count = 0;
    while (count<n) {
        char current_line[1024];
        int n = read(input_fd, current_line, sizeof(current_line));
        if (n <= 0) {
            break; // End of file or error
        }
    cprintf("%s\n", current_line);
        count++;
    }
}

int main(int argc, char *argv[]) {

    int lines = 4;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            lines = atoi(argv[i+1]);
        } else {
            printf(2, "Usage: uniq [-c] [-i] [-d] < inputfile > outputfile\n");
            exit();
        }
    }
    
    printf(2, "Uniq command is getting executed in kernel mode.\n");
    
    // Invoke the uniq system call
    print_head_lines(0,lines);

    exit();
}
