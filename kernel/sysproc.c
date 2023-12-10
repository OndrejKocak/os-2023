#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_mmap(void){
  uint64 addr, len, offset;
  int prot, flags, fd;
  struct vm_area_struct* vma = 0;
  argaddr(0, &addr);
  argaddr(1, &len);
  argint(2, &prot);
  argint(3, &flags);
  argint(4, &fd);
  argaddr(5, &offset);
  struct proc *p = myproc();
  len = PGROUNDDOWN(len);
  if(p->sz+len > MAXVA || offset <0 || offset%PGSIZE){
    return -1;
  }
  for(int i = 0; i <NVMA; i++){
    if(p->vma[i].addr) continue;
    vma = &p->vma[i];
    break;
  }
  if(vma){
    
    return -1;
  }
  if(addr == 0){
    vma->addr = p->sz;
  }
  else{
    vma->addr = addr;
  }
  vma->len = len;
  vma->prot = prot;
  vma->fd = fd;
  vma->flags = flags;
  vma->offset = offset;
  vma->file = p->ofile[fd];
  filedup(vma->file);
  p->sz +=len;
  return (uint64)-1;
}

uint64 sys_munmap(void){
  uint64 addr, len;
  argaddr(0, &addr);
  argaddr(1, &len);
  return (uint64)-1;
}

void* mmap(void* addr, uint64 length, int prot, int flags, int fd, uint64 offset){
  return(void*)-1;
}