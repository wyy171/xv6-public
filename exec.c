// In exec.c

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  uint sz, sp, up, hp, cp;
  int i, argc;
  char *s, *last;

  // Open the file
  begin_op();
  if ((ip = namei(path)) == 0) {
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);

  // Check ELF header
  if (readi(ip, (char*)&ph, 0, sizeof(ph)) != sizeof(ph)) {
    goto bad;
  }
  if (ph.magic != ELF_MAGIC) {
    goto bad;
  }

  // Setup kernel page table
  pgdir = 0;
  if ((pgdir = setupkvm()) == 0) {
    goto bad;
  }

  // Load program into memory
  sz = 0;
  for (i = 0; i < ph.phnum; i++) {
    if (readi(ip, (char*)&ph, ph.phoff + i * sizeof(ph), sizeof(ph)) != sizeof(ph)) {
      goto bad;
    }
    if (ph.type != ELF_PROG_LOAD) {
      continue;
    }
    if (ph.memsz < ph.filesz) {
      goto bad;
    }
    if (ph.vaddr + ph.memsz < ph.vaddr) {
      goto bad;
    }
    if ((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0) {
      goto bad;
    }
    if (ph.vaddr % PGSIZE != 0) {
      goto bad;
    }
    if (loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0) {
      goto bad;
    }
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible. Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if ((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0) {
    goto bad;
  }
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Set the starting address of the stack, heap, and code
  up = 0;
  hp = sp - PGSIZE * 5;  // Leave at least 5 pages unallocated between stack and heap
  cp = hp - PGSIZE;       // Code starts right before the heap

  // Allocate a page for the stack
  if (allocuvm(pgdir, sp - PGSIZE, sp) == 0) {
    goto bad;
  }

  // Allocate a gap of at least 5 pages
  for (char *va = (char *)(sp - PGSIZE); va > (char *)hp; va -= PGSIZE) {
    if (allocuvm(pgdir, (uint)va - PGSIZE, (uint)va) == 0) {
      goto bad;
    }
  }

  // Allocate a page for the heap
  if (allocuvm(pgdir, hp - PGSIZE, hp) == 0) {
    goto bad;
  }

  // Allocate a page for the code
  if (allocuvm(pgdir, cp - PGSIZE, cp) == 0) {
    goto bad;
  }

  // Set the program's data pages to be invalid
  for (char *va = (char *)(cp - PGSIZE); va > (char *)up; va -= PGSIZE) {
    // Use walkpgdir to get the page table entry
    pte_t *pte = walkpgdir(pgdir, va, 0);
    if (pte == 0) {
      goto bad;
    }
    *pte &= ~PTE_P; // Mark the page table entry as invalid (clear the PTE_P bit)
  }

  // Push argument strings, prepare rest of stack in ustack.
  for (argc = 0; argv[argc]; argc++) {
    if (argc >= MAXARG) {
      goto bad;
    }
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if (copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;
  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));


  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  curproc->stack_addr = STACKBASE - PGSIZE;
  //curproc->stack_addr = curproc->stack_addr - PGSIZE;
  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
