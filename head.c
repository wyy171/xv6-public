#include "types.h"
#include "stat.h"
#include "user.h"

int
head_n(int input_fd, int lines) {

    int count = 0;
    char buf[512];  

   
    int n = read(input_fd, buf, sizeof(buf));
        
    if (n <= 0) {
            return -1; // End of file or error
    }

    // Null-terminate the buf
    buf[n] = '\0';
    int i = 0;

    while (buf[i]!='\0' && count < lines) {
        char current_line[512] = "";   //initial the current line
        int j;
        for (j = 0; buf[i]!='\0' && buf[i]!='\n'; j++, i++) {
            current_line[j] = buf[i];
        }

        current_line[j+1] = '\n';
        
        printf(1, "%s\n", current_line);
       
        count ++; // Increment the count for the lines
        i++; //skip '\n'
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
