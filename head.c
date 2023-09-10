#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void print_head_lines(int input_fd, int n){
    int count = 0;
    printf(2, "lines = %d\n", n);
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
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <n>\n", argv[0]);
        exit(1);
    }

    char *filename = argv[1];
    int n = atoi(argv[2]);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    print_head_lines(fd, n);

    close(fd);

    exit();
}
