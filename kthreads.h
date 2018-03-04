//spin lock
typedef struct lock_t
{
  uint locked;
}lock_t;
// kernal thread 
typedef struct kthread_t
{
  uint pid; // thread id
  void* stack;
}kthread_t;
