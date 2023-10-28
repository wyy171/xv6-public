#include "types.h"
#include "user.h"

// Function to simulate a long-running task
void long_task(int priority, char* program) {
    int pid = fork();
    if (pid < 0) {
        printf(1, "Fork failed.\n");
    } else if (pid == 0) {
        // Child process
        setpr(pid, priority); // Set the priority
        char *argv[] = { "uniq", "input.txt", 0 }; // Adjust the arguments as needed
        exec(program, argv);
        
        if (exec(program, argv) < 0) {
            printf(1, "Exec failed for %s\n", program);
        }
        else 
            printf(1, "Exec succeed for %s\n", program);
        exit();
    }
}

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
        char* program = i == 0 ? "uniq" : "uniq_kernel"; // Run uniq for user and kernel
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
        char* program = i == 0 ? "uniq" : "uniq_kernel"; // Run uniq for user and kernel
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
