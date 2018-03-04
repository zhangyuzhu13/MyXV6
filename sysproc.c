#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "procinfo.h"
int
sys_fork(void)
{
  return fork();
}

int
sys_getprocsinfo(void)
{
  struct procinfo *info;
  if(argptr(0, (void*)&info, NPROC*sizeof(*info)) < 0)
    return -1;
  return getprocsinfo(info);
}

int
sys_shmem_access(void) 
{
  int pgindex;
  if(argint(0, &pgindex) < 0)
    return 0;
  return shmem_access(pgindex);
}

int 
sys_shmem_count(void)
{
  int pgindex;
  if(argint(0,&pgindex) < 0)
    return -1;
  return shmem_count(pgindex);
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
// create a thread
int 
sys_clone(void)
{
  char* fcn = 0;
  char* arg = 0;
  char* stack = 0;
  if(argptr(0, &fcn, sizeof(char*)) < 0 || argptr(1, &arg, sizeof(char*)) < 0 || argptr(2, &stack, PGSIZE) < 0){
    return -1;
  }
  return clone((void*)fcn, (void*)arg, (void*)stack);
}
// thread to join
int
sys_join(void)
{
  int pid;
  if(argint(0, &pid) < 0){
    return -1;
  }
  return join(pid);
}
