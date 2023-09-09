#include "types.h"
#include "stat.h"
#include "user.h"


void print_head_lines(int input_fd, int n){
    int count = 0;
    while (count<n) {
        char current_line[1024];
        int m = read(input_fd, current_line, sizeof(current_line));
        if (m <= 0) {
            break; // End of file or error
        }
        current_line[m] = '\0';
        printf(2, "%s\n", current_line);
        count++;
    }
}

int main(int argc, char *argv[]) {

    int lines = 4;
     printf(2, "%d, %s, %s", argc, argv[1],argv[2]);
    // Parse command-line arguments
    if(argc == 3)
        lines = atoi(argv[2]);
    
    printf(2, "Uniq command is getting executed in kernel mode.\n");
    
    // Invoke the uniq system call
    print_head_lines(0,lines);

    exit();
}
