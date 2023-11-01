#include "types.h"
#include "user.h"

// Function to simulate a long-running task
void long_task(int priority, char* program, char *file_name) {
    int pid = fork();
    if (pid < 0) {
        printf(1, "Fork failed.\n");
    } else if (pid == 0) {
        // Child process
        setpr(pid, priority); // Set the priority
        char *argv[] = { program, file_name, 0 }; // Adjust the arguments as needed
       
        exec(program, argv);
        
        if (exec(program, argv) < 0) {
            printf(1, "Exec failed for %s\n", program);
        }
        else 
            printf(1, "Exec succeed for %s\n", program);
        exit();
    }
    //else
        //wait();
}


int main(int argc, char *argv[]) {
    if (argc < 2 || (argc - 1) % 3 != 0) {
        printf(2, "Usage: %s [process_name file_name priority]...\n", argv[0]);
        exit();
    }

    int num_processes = (argc - 1) / 3;
    int wait_time = 0;
    int turnaround_time = 0;

    // FCFS Scheduling
    printf(1, "FCFS Scheduling:\n");
    for (int i = 0; i < num_processes; i++) {
        char *program = argv[1 + 3 * i];
        char *file_name = argv[2 + 3 * i];
        //int priority = atoi(argv[3 + 3 * i]);
        
        //char* program = i == 0 ? "uniq" : "head"; // Run uniq for user and kernel
        long_task(1, program, file_name); // Priority 1 for FCFS
   
           // Measure start time of process
        int start_time = uptime();

        // Wait for the child process to finish
        int ctime, etime;
        waitx(&ctime, &etime);      

        // Measure finish time of process
        int finish_time = uptime();

        wait_time += start_time-ctime;
        turnaround_time += finish_time-ctime;
        
    }
    // Calculate average wait and turnaround times for FCFS
    int avg_wait_time_fcfs = wait_time / num_processes;
    int avg_turnaround_time_fcfs = turnaround_time / num_processes;

    // Priority-Based Scheduling
    printf(1, "\nPriority-Based Scheduling:\n");
    wait_time = 0;
    turnaround_time = 0;
/*
    for (int i = 0; i < num_processes; i++) {
        
        char *program = argv[1 + 3 * i];
        char *file_name = argv[2 + 3 * i];
        int priority = atoi(argv[3 + 3 * i]);
        long_task(priority, program, file_name); 
   
    }
     // Wait for child processes to complete
    for (int i = 0; i < num_processes; i++) {
        
           // Measure start time of process
        int start_time = uptime();

        // Wait for the child process to finish
        int ctime, etime;
        waitx(&ctime, &etime);      

        // Measure finish time of process
        int finish_time = uptime();

        wait_time += start_time-ctime;
        turnaround_time += finish_time-ctime;
        
    }
*/
    char *program = argv[1 ];
    char *file_name = argv[2 ];
    int priority = atoi(argv[3 ]);
    long_task(priority, uniq, file_name); 

    char *program = argv[4 ];
    char *file_name = argv[5 ];
    int priority = atoi(argv[6 ]);
    long_task(priority, uniq, file_name); 

    int start_time = uptime();

        // Wait for the child process to finish
        int ctime, etime;
        waitx(&ctime, &etime);      

        // Measure finish time of process
        int finish_time = uptime();

        wait_time += start_time-ctime;
        turnaround_time += finish_time-ctime;

    int start_time = uptime();

        // Wait for the child process to finish
        int ctime, etime;
        waitx(&ctime, &etime);      

        // Measure finish time of process
        int finish_time = uptime();

        wait_time += start_time-ctime;
        turnaround_time += finish_time-ctime;
    
    // Calculate average wait and turnaround times for Priority-Based Scheduling
    int avg_wait_time_priority = wait_time / num_processes;
    int avg_turnaround_time_priority = turnaround_time / num_processes;

    // Report statistics
    printf(1, "\nAverage Wait Time (FCFS): %d\n", avg_wait_time_fcfs);
    printf(1, "Average Turnaround Time (FCFS): %d\n", avg_turnaround_time_fcfs);

    printf(1, "Average Wait Time (Priority-Based): %d\n", avg_wait_time_priority);
    printf(1, "Average Turnaround Time (Priority-Based): %d\n", avg_turnaround_time_priority);

    exit();
}
/*int main(int argc, char *argv[]) {
    
    for (int i = 0; i < num_processes; i++) {
        char *process_name = argv[1 + 2 * i];
        int priority = atoi(argv[2 + 2 * i);

        int pid = fork();

        if (pid < 0) {
            printf(2, "Fork failed\n");
            exit();
        } else if (pid == 0) {
            // Child process
            //setpriority(priority);
            setpr(priority);
            // Execute the process using exec()
            char *args[] = {process_name, 0};
            exec(process_name, args);

            printf(2, "Exec failed\n");
            exit();
        }
    }

    // Wait for child processes to complete
    for (int i = 0; i < num_processes; i++) {
        wait(0);
    }

    exit();
}

*/
/*
int main(int argc, char *argv[]) {
    if (argc != 1) {
        printf(1, "Usage: test\n");
        exit();
    }

    int num_processes = 2;
    int wait_time = 0;
    int turnaround_time = 0;

    // FCFS Scheduling
    printf(1, "FCFS Scheduling:\n");
    for (int i = 0; i < num_processes; i++) {
        char* program = i == 0 ? "uniq" : "head"; // Run uniq for user and kernel
        long_task(1, program); // Priority 1 for FCFS

        // Measure start time of process
        int start_time = uptime();

        // Wait for the child process to finish
        wait();

        // Measure finish time of process
        int finish_time = uptime();

        wait_time += start_time;
        turnaround_time += finish_time;
    }

    // Calculate average wait and turnaround times for FCFS
    int avg_wait_time_fcfs = wait_time / num_processes;
    int avg_turnaround_time_fcfs = turnaround_time / num_processes;

    // Priority-Based Scheduling
    printf(1, "\nPriority-Based Scheduling:\n");
    wait_time = 0;
    turnaround_time = 0;

    for (int i = 0; i < num_processes; i++) {
        char* program = i == 0 ? "uniq" : "head"; // Run uniq for user and kernel
        long_task(i + 2, program); // Priority 2 and 3

        // Measure start time of process
        int start_time = uptime();

        // Wait for the child process to finish
        wait();

        // Measure finish time of process
        int finish_time = uptime();

        wait_time += start_time;
        turnaround_time += finish_time;
    }

    // Calculate average wait and turnaround times for Priority-Based Scheduling
    int avg_wait_time_priority = wait_time / num_processes;
    int avg_turnaround_time_priority = turnaround_time / num_processes;

    // Report statistics
    printf(1, "\nAverage Wait Time (FCFS): %d\n", avg_wait_time_fcfs);
    printf(1, "Average Turnaround Time (FCFS): %d\n", avg_turnaround_time_fcfs);

    printf(1, "Average Wait Time (Priority-Based): %d\n", avg_wait_time_priority);
    printf(1, "Average Turnaround Time (Priority-Based): %d\n", avg_turnaround_time_priority);

    exit();
}
    */
