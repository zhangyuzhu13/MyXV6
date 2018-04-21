#include "types.h"
#include "user.h"
#include "stat.h"
#include "param.h"
#include "x86.h"
#include "kthreads.h"

#define PGSIZE (4096)
// lock functions
void init_lock(struct lock_t* lk)
{
  lk->locked = 0; // mark as unlocked when initial
}

void lock_acquire(struct lock_t* lk)
{
  while (xchg(&lk->locked, 1) != 0); // xchg is atomic exchange in x86
   // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();  
}

void lock_release(struct lock_t* lk) 
{
  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();
  xchg(&lk->locked, 0);
}

// thread functions
struct kthread_t thread_create(void(*start_routine)(void*), void *arg)
{
  int stack_addr;
  struct lock_t lk;
  init_lock(&lk);
  // add lock when malloc
  lock_acquire(&lk);
  void* stack = malloc(PGSIZE);
  // when the stack is not page aligned
  if((uint)stack % PGSIZE != 0){
    free(stack);
    stack = malloc(2*PGSIZE);
    stack_addr = (uint)stack;
    stack = stack + (PGSIZE - (uint)stack % PGSIZE);
  }
  else {
    stack_addr = (uint)stack;
  }
  lock_release(&lk);
  //((uint*)stack)[0] = stack_addr;
  int pid = clone(start_routine, arg, stack);
  struct kthread_t ktd;
  ktd.pid = pid;
  ktd.stack = (void*)stack_addr;
  return ktd;
}

int thread_join(struct kthread_t ktd)
{
  lock_t lk;
  init_lock(&lk);
  int pid = join(ktd.pid);
  
  lock_acquire(&lk);
  free(ktd.stack);
  lock_release(&lk);
  return pid;
}
