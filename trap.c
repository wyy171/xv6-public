#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

void
trap(struct trapframe *tf)
{
  struct proc *curproc = myproc();//TODO
  if(tf->trapno == T_SYSCALL){
    if(curproc->killed)
      exit();
    curproc->tf = tf;
    syscall();
    if(curproc->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  case T_PGFLT:
    // Page fault case. Allocate new page for stack if possible.
    if(curproc == 0 || (tf->cs&3) == 0){
        cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
        tf->trapno, cpuid(), tf->eip, rcr2());
        panic("trap");
    } else {
        //   cprintf("proc->stack_sz:%x proc->sz:%x\n",proc->stack_sz, proc->sz);
        uint st_address = rcr2();
        if (st_address <= curproc->stack_sz && st_address >= curproc->stack_sz-PGSIZE && curproc->stack_sz-PGSIZE*6 >= curproc->sz) {
            if((allocuvm(curproc->pgdir, curproc->stack_sz-PGSIZE, curproc->stack_sz)) != 0) {
                curproc->stack_sz -= PGSIZE;
                return;
            }
        }else{
            // Segmentation fault. Kill process.
            cprintf("pid %d %s: trap %d err %d on cpu %d "
                "eip 0x%x addr 0x%x--kill proc\n",
                curproc->pid, curproc->name, tf->trapno, tf->err, cpuid(), tf->eip,
            rcr2());
            curproc->killed = 1;
        }
    }
    break;

  default:

    if(curproc == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            curproc->pid, curproc->name, tf->trapno, tf->err, cpuid(), tf->eip,
            rcr2());
    curproc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(curproc && curproc->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(curproc && curproc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(curproc && curproc->killed && (tf->cs&3) == DPL_USER)
    exit();
}
