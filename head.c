#include "defs.h"
#include "file.h"
#include "fcntl.h"
#include "param.h"
#include "stat.h"

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
void khead(int fd, int n) {
    char buf[512];
    int count = 0;

    while (count < n) {
        int nread = read(fd, buf, sizeof(buf));
        if (nread <= 0)
            break;

        for (int i = 0; i < nread; i++) {
            char c = buf[i];
            putchar(c); // Replace with your kernel-specific output function
            if (c == '\n')
                count++;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cprintf("Usage: myhead <file> <n>\n");
        exit();
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        cprintf("myhead: cannot open %s\n", argv[1]);
        exit();
    }

    int n = atoi(argv[2]);
    khead(fd, n);

    close(fd);

    exit();
}
