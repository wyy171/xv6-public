#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_uniq(void) {
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
                printf(output_fd, "%d ", count);
                printf(output_fd, "%s\n", prev_line);
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
        //strcpy(prev_line, current_line);
        while (*current_line != '\0') {
           *prev_line = *current_line 
           prev_line++;
           current_line++;// Copy characters from current_line to prev_line until a null terminator is encountered
        }
    }

    // Handle the last line (if any)
    if (count > 0) {
        if (cflag) {
            // Output the count and line if -c flag is set
            printf(output_fd, "%d ", count);
            printf(output_fd, "%s\n", prev_line);
        } else if (!dflag) {
            // Output the unique line (if not using -d)
            printf(output_fd, "%s\n", prev_line);
        }
    }

    return 0; // Success
}
