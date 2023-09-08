#include "types.h"
#include "stat.h"
#include "user.h"

char my_tolower(char c) {
    if (c >= 'A' && c <= 'Z') {
        // Convert uppercase to lowercase by adding the ASCII offset
        return c + ('a' - 'A');
    } else {
        // Leave non-uppercase characters unchanged
        return c;
    }
}

int my_strcmp(const char str1[1024], const char str2[1024]) {
    int i = 0;

    while (str1[i] != '\0' || str2[i] != '\0') {
        if (str1[i] < str2[i])
            return -1;
        else if (str1[i] > str2[i])
            return 1;
        
        i++;
    }
    
    return 0;
}

void
uniq(void) {
    int input_fd, cflag, iflag, dflag;

    // Retrieve arguments from the user stack
    if (argint(0, &input_fd) < 0 || 
        argint(1, &cflag) < 0 || argint(2, &iflag) < 0 || argint(3, &dflag) < 0) {
        return; // Error in retrieving arguments
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
                current_line[i] = my_tolower(current_line[i]);
            }
        }

        // Implement -d flag logic (skip duplicate lines)
        if (!dflag || my_strcmp(current_line, prev_line) != 0) {
           if (count > 0) {
                // Output the count and line if -c flag is set
                cprintf("%d", count);
                cprintf("%s", prev_line);
            
            } else if (count == 0 && !dflag) {
                // Output the unique line (if not using -d)
               cprintf("%s", prev_line);
            }
            
            // Reset the count for the new line
            count = 1;
        } else {
            // Increment the count for duplicate lines
            count++;
        }

        // Update prev_line
        //strcpy(prev_line, current_line);
        for (int i=0; current_line[i] != '\0'; i++) {
           prev_line[i] = current_line[i]; 
          }
    }

    // Handle the last line (if any)
    if (count > 0) {
        if (cflag) {
            // Output the count and line if -c flag is set
             cprintf("%d", count);
             cprintf("%s", prev_line);
        } else if (!dflag) {
            // Output the unique line (if not using -d)
            cprintf("%s", prev_line);
        }
    }

    return; // Success
}

void
head(void) {
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
    printf("Uniq command is getting executed in kernel mode.\n");
    
    // Invoke the uniq system call
    int ret = uniq(0, 1, cflag, iflag, dflag);

    if (ret < 0) {
        printf(2, "uniq: syscall failed\n");
    }

    exit();
}

