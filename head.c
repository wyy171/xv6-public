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
    printf(2, "%s\n", current_line);
        count++;
    }
}

int main(int argc, char *argv[]) {

    int lines = 4;

    // Parse command-line arguments
    if(argv[0] > 1)
        lines = atoi(argv[2]);
    
    printf(2, "Uniq command is getting executed in kernel mode.\n");
    
    // Invoke the uniq system call
    print_head_lines(0,lines);

    exit();
}
