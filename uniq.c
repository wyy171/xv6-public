#include "types.h"
#include "stat.h"
#include "user.h"

int
uniq(void) {
    int input_fd, output_fd, cflag, iflag, dflag;

    // Retrieve arguments from the user stack
    if (argint(0, &input_fd) < 0 || argint(1, &output_fd) < 0 ||
        argint(2, &cflag) < 0 || argint(3, &iflag) < 0 || argint(4, &dflag) < 0) {
        return -1; // Error in retrieving arguments
    }

    char prev_line[1024] = "";
    int count = 0;

    while (1) {
        char current_line[1024];
        int n = read(input_fd, current_line, sizeof(current_line));
        
        if (n <= 0) {
            break; // End of file or error
        }

        // Null-terminate the line
        current_line[n] = '\0';

        // Implement case-insensitive comparison if -i flag is set
        if (iflag) {
            for (int i = 0; current_line[i]; i++) {
                current_line[i] = tolower(current_line[i]);
            }
        }

        // Implement -d flag logic (skip duplicate lines)
        if (!dflag || strcmp(current_line, prev_line) != 0) {
            if (count > 0) {
                // Output the count and line if -c flag is set
                dprintf(output_fd, "%d %s\n", count, prev_line);
            } else if (count == 0 && !dflag) {
                // Output the unique line (if not using -d)
                dprintf(output_fd, "%s\n", prev_line);
            }
            
            // Reset the count for the new line
            count = 1;
        } else {
            // Increment the count for duplicate lines
            count++;
        }

        // Update prev_line
        strcpy(prev_line, current_line);
    }

    // Handle the last line (if any)
    if (count > 0) {
        if (cflag) {
            // Output the count and line if -c flag is set
            dprintf(output_fd, "%d %s\n", count, prev_line);
        } else if (!dflag) {
            // Output the unique line (if not using -d)
            dprintf(output_fd, "%s\n", prev_line);
        }
    }

    return 0; // Success
}

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
    printf(2, "Uniq command is getting executed in kernel mode.\n");
    
    // Invoke the uniq system call
    int ret = uniq(0, 1, cflag, iflag, dflag);

    if (ret < 0) {
        printf(2, "uniq: syscall failed\n");
    }

    exit();
}

