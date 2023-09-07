#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 1024

void print_unique_lines(FILE *file,bool display_counts,bool ignore_case,bool display_duplicates){
    char prev_line[MAX_LINE_LENGTH] = "";
    char current_line[MAX_LINE_LENGTH];
    int count = 1; // Count for consecutive duplicate lines
    if(display_counts == false && ignore_case == false && display_duplicates == false)
        while (fgets(current_line, sizeof(current_line), file) != NULL) {
            if (strcmp(prev_line, current_line) != 0) {
                printf("%s", current_line);
                strcpy(prev_line, current_line);
            }
        }
    if(display_counts == true && ignore_case == false && display_duplicates == false){
        while (fgets(current_line, sizeof(current_line), file) != NULL) {
            if (strcmp(prev_line, current_line) != 0) {
                if(prev_line[0] != '\0')
                    printf("%d %s", count, prev_line);   // Output the count and line if -c flag is set
                strcpy(prev_line, current_line);
                count = 1;  // Reset the count for the new line
            }
            else count++;   // Increment the count for duplicate lines
        }
        printf("%d %s", count, prev_line);
    }
    if(display_counts == false && ignore_case == true && display_duplicates == false)
        while (fgets(current_line, sizeof(current_line), file) != NULL) {
            // Implement case-insensitive comparison if -i flag is set
            if (strcasecmp(prev_line, current_line) != 0) {
                printf("%s", current_line);
                strcpy(prev_line, current_line);
            }
        }
    if(display_counts == false && ignore_case == false && display_duplicates == true){
        while (fgets(current_line, sizeof(current_line), file) != NULL) {
            if (strcmp(prev_line, current_line) != 0) {
                if(prev_line[0] != '\0' && count!=1)
                    printf("%s", prev_line);    // Output the unique line if -d flag is set
                strcpy(prev_line, current_line);
                count = 1;
            }
            else count++;
        }
        if(count!=1)
            printf("%s", prev_line);
    }
}

int main(int argc, char *argv[]) {
    
    bool display_counts = false;
    bool ignore_case = false;
    bool display_duplicates = false;
    char *filename = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            display_counts = true;
        } else if (strcmp(argv[i], "-i") == 0) {
            ignore_case = true;
        } else if (strcmp(argv[i], "-d") == 0) {
            display_duplicates = true;
        } else {
            // Assume it's a filename
            filename = argv[i];
        }
    }
    
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    printf("Uniq command is getting executed in user mode.\n");
    print_unique_lines(file,display_counts,ignore_case,display_duplicates);
    

    fclose(file);

    return EXIT_SUCCESS;
}
