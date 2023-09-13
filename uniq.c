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

int
uniq_compare(int input_fd, int output_fd, int cflag, int iflag, int dflag) {

    char prev_line[1024] = "";
    int count = 0;
    char buf[512];  

   
    int n = read(input_fd, buf, sizeof(buf));
        
    if (n <= 0) {
            return -1; // End of file or error
    }

    // Null-terminate the buf
    buf[n] = '\0';
    int i = 0;

    printf(output_fd, "buf = %s\n", buf);
     while (1) {
        char current_line[1024];
        printf(1, "i = %d\n", i);
         printf(output_fd, "prev_line = %s\n", prev_line);
         printf(output_fd, "current_line = %s\n", current_line);
        for (int j = 0; buf[i]!='\0' && buf[i]!='\n'; j++, i++) {
                current_line[j] = buf[i];
            }
         printf(1, "i1 = %d\n", i);
         printf(output_fd, "prev_line1 = %s\n", prev_line);
         printf(output_fd, "current_line1 = %s\n", current_line);
        // Implement case-insensitive comparison if -i flag is set
        if (iflag) {
            for (int k = 0; current_line[k]; k++) {
                current_line[k] = my_tolower(current_line[k]);
            }
        }

        // Implement -d flag logic ( It only prints the repeated lines and not the lines which aren’t repeated.)
        if (!dflag || strcmp(current_line, prev_line) != 0) {
            if (count > 0) {
                // Output the count and line if -c flag is set
                printf(output_fd, "%d %s\n", count, prev_line);
            } else if (count == 0 && !dflag) {
                // Output the unique line (if not using -d)
                printf(output_fd, "%s\n", prev_line);
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
            printf(output_fd, "%d %s\n", count, prev_line);
        } else if (!dflag) {
            // Output the unique line (if not using -d)
            printf(output_fd, "%s\n", prev_line);
        }
    }

    return 0; // Success
}

int main(int argc, char *argv[]) {
    uniq();
    int cflag = 0, iflag = 0, dflag = 0, fd=0;
    int ret;
    
       printf(1, "Uniq command is getting executed in user mode.\n");
    // Process command-line arguments
     if(argc < 2){
        //uniq_read(0,count, only_same, ignore_case);
        ret = uniq_compare(0, 1, cflag, iflag, dflag);
    }
    if(argc == 2){
        if((fd = open(argv[1],0)) < 0){
            printf(1, "uniq: cannot open %s \n", argv[1]);
            exit();
        }
        //uniq_read(fd, count, only_same, ignore_case);
        ret = uniq_compare(fd, 1, cflag, iflag, dflag);
        close(fd);
        exit();
    }
    if(argc > 2){
          if(strcmp(argv[1], "-c") == 0){
            cflag = 1;
            }
          if(strcmp(argv[1], "-d") == 0){
            dflag = 1;
            }
          if(strcmp(argv[1], "-i") == 0){
            iflag = 1;
          }
          if((fd = open(argv[2],0)) < 0){
            printf(1, "uniq: cannot open %s \n", argv[2]);
            exit();
          }
          //uniq_read(fd, count, only_same, ignore_case);
          ret = uniq_compare(fd, 1, cflag, iflag, dflag);
          close(fd);
    }

    if (ret < 0) {
        printf(2, "uniq: syscall failed\n");
    }

    exit();
}

