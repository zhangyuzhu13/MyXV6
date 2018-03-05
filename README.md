# MyXV6 #

### 1.System call -- getprocsinfo ###

Based on the xv6 System, I implemented a system call named getprocsinfo.
The function prototype is:

`int getprocsinfo(struct procinfo*)`

struct procinfo is a structure with two data members, an integer pid and a string pname. The syscall should write the data into the passed argument for each existing process at the time of the call. The syscall should return the number of existing processes, or -1 if there is some error.

I added a procinfo.h to put the struct procinfo, included this head file in proc.c, sysproc.c, testgetprocsinfo.c, declared the procinfo struct in proc.h, user.h. 
In the usys.S file, add "SYSCALL(getprocsinfo)" to link the system call.
In the user.h file, I declared a function:

 ` int getprocsinfo(struct procinfo*);`
 
In the syscall.h, I defined a system call number:

`#define SYS_getprocsinfo 22`

In the syscall.c file, I combine the SYS_getprocsinfo with system call function sys_getprocsinfo:

`  extern int sys_getprocsinfo(void) `
`  [SYS_getprocsinfo]  sys_getprocsinfo`

 In the sysproc.c file, I implement the system call function sys_getprocsinfo(void) to check the argument and call for the low level system call function getprocsinfo():

```
	int
	sys_getprocsinfo(void)
	{
  		struct procinfo *info;
  		if(argptr(0, (void*)&info, NPROC*sizeof(*info)) < 0)
    		return -1;
  		return getprocsinfo(info);
	}
```

In the proc.c file, I impelemented the low level getprocsinfo function:

```
	int
	getprocsinfo(struct procinfo* info)
	{
  		struct proc *p;
  		struct procinfo *in;
  		in = info; 
  		int count = 0;
  		acquire(&ptable.lock);
  		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  		{
    		if (p->state == EMBRYO || p->state == RUNNABLE || p->state == RUNNING || p->state == SLEEPING)
			{
	  			in->pid = p->pid;
	  			safestrcpy(in->pname, p->name,16);
	  			count++;
	  			in++;
			}
  		}
  		release(&ptable.lock);
  		return count;
	}
```

I implemented the test case in testgetprocsinfo.c file.
Finally, modify the Makefile to add the testgetprocsinfo.c into it.
Considering the method I impelemnted my system call is only add my own method into xv6 kernal, there is nothing I have done to modify the oriangl code, so it could have minimum impact on kernal overhead.

To run my xv6 system in qemu, the command is:

```
qemu-system-i386 -serial mon:stdio -hdb fs.img xv6.img -smp 2 -m 512
```

To run my test file, the command is:
```
testgetprocsinfo
```

## 2. Null pointer dereference ##

In xv6, the user program text are loaded into the very first part of address space with virtual address 0. Thus when dereference a null pointer which is point to the 0 virtual address, we will get a return value  which ia actually the program text. To fix this problem, what I have done is to load program code in "next" virtual page and copy the text to the "next" page. Finally we need to set the program entry at the "next" page. 
I have modified the following place:

* exec.c: load an empty page at address 0 and leave it there, copy our code and data begin at the second page. 
* proc.c: copyuvm() should copy begin at the second page.
* syscall.c: argptr() the pointer address should not be 0.
* Makefile: change the user program' entry to 0x1000 not 0.

## 3. Share memory ##

Share memory is used for different processes to communicate with each other. Each process can read or write in the share memory. The value in the share memory can be reaches by all process. 
The 

I define and implement a function `shmeminit()` which is used to allocate 4 page memory for sharing. Define a gloable array to save these four memory pages' virtual address and the number process using this share memory.Then call this `shmeminit()` during booting in `main.c`. Then define and implement function `shmem_access()` and `shmem_count` function. Then modify the `fork()` for copy share memory address from parent process to son process. Then modify `freevm()` to keep share memory not be free. Modify the `allocuvm()` to check boundary not get into the share memory address. 

The details I change is showing below:

* vm.c: implement functions `shmeminit()` , `shmem_access()`, `shmem_count()`. Modify `freevm()` not to free share memory page. 
* proc.c: Modify `fork()` to copy the share memory address. Modify `wait()` function to count the process number of using share memory. Modify `allocuvm()` to check the boundary not get into the share memory address.
* syscall.c, syscall.h, defs.h, user.h, sysproc.c: add system call sys_shmem_access and sys_shmem_count
* main.c: add the `shmeminit()` function in the `main()` to initial the share memory.

##### The implementation of `shmem_access(int index)` function #####

* if the share memory address of this index have already in the process page table, then return it directly.
* Start from top of virtual address in user space(0x80000000 which is KERNBASE)
* The first virtual address of share memory should be starting at KERNBASE - PGSIZE.
* Walk	through the page	table and	map virtual addresses to their physical addresses.
* Physical can be obtain form our global array which is used to track the share memory virtual address.

##### The implementation of `shmem_count(int index)` function #####

* Initialize the count of process using share memory as 0 in `shmeminit()`
* When map this share memory into the process page table. The count should add 1.
* When fork() is being called, the share memory used by father process should also used by son process, so each should add 1.
* When wait() is being called, the son process has finished, so the share memory  is not being used by this process, it should minus 1.

##### Test case #####
I write a test case using twice fork(). The process 1 read the value in share memory. The process 2 change the value in share memory, the process 3 set the value in share memory. Then using `wait()` to let process runing order as 3->2->1.
So the result of process 3 should be the value it set. The result of process 2 should be the value it changed. The result of process 1 should be the same one of process 2.
Call the `shmem_count()` in each process, and because of the `wait()`, all of the number should be 1.

To run the share memory test case, use commond: `testmemory`

## 4. Thread

In this part, I added the kernal thread into xv6 by implement two system call `clone()` and `join()`. The `clone()` is used to create a kernal thread and the `join()` is called to wait for a thread. Then I used these two system call to create a thread library with several functions so that user can use these library to use threads in their programs. Finally, I use the test case to test what I have implement and get the right result.

### The main idea

I just want to use the proc structure which has already been there to represent both the thread and the process. In this way I could save much time to deal with the thread schedule. So I would add some field into the proc structure to distinguish it's a process or a thread. Also because of the way we represent the thread, the thread pid will be different even in different process. That means we could use pid to get the exact thread we want.  

When implementing the clone() system call, the fields in proc structure I should change is the information about stack. There are four registers I need to know them well and change them into the proper value. 

The three register is `esp`, `ebp`, `eip`, `eax`. The `esp` keeps the high address of the stack. The `ebp` keeps the low address of the stack. The stack grows from high address to the low address. The `eip` keeps the address of the function entry which will be execute by this thread. The `eax` keeps the pid.  

The high address of stack should keep the pointer of argument then the return value. That means the `esp` and the `ebp` will start at the high address - 2 * sizeof(uint). The high address - sizeof(uint) keeps the argument pointer. The high address - 2 * sizeof(uint) keeps the return value which is 0xffffffff in our requirment. The `eip` should just be same with the pointer got from the parameter. The `eax` should return the its own pid if it is a thread which is different from process.

The `join()` could be similar with wait() but just deal with thread. If it's not thread then just ignore.

The key of the lock part is the atomic exchage `xchg(&lk->locked, 1)`. This atomic operation can make sure only one thread could get the lock.

There are some more details I will talk about in the rest of this part. 

### struct proc

First I add two more fields in the proc structure.

* int isthread
* void* stack

The first field `isthread` is used to distinguish that a proc instance is a thread or a process. 

### struct kthread_t

* int pid
* void* stack

We keep a pointer to stack at this struct so that we can use this pointer to free this stack in user space. In this way we can avoid the memory leak.

### clone() system call

The structure of clone() function would be like this:
`int clone(void(*fcn) (void*), void *arg, void*stack)`
In the `clone()` system call, I first get the proc which is running now and copy most of the fields from the proc to the thread. What I chaneg is four register. The high address of stack should be (stack + PGSIZE - sizeof(uint)). We should put arg pointer at this address. Then put return value 0xffffffff into (stack + PGSIZE - 2 * sizeof(uint)). The `esp` and `ebp` will also initialize using the return value address. The `eax` register will keep the pid of the thread.

### join() system call

Most of the `join()` part is similar with `wait()`. What different is: DO NOT FREE PAGE TABLE IF THIS IS A  THREAD. 

### thread_create() and thread_join() function

In this function, we try to malloc a PGSIZE memory as the stack of new thread. When the address is not page aligned, we need to make the 2 PGSIZE then cut the part out of a page. Then we create a kthread_t instance which have the stack pointer and pid and return this instance.
In thread_join(), we just call join() system call then remember to free the thread stack.

### Details of change 

####proc.c

allocproc(): add `p->isthread = 0` which means we initialize it as process. We should change this into 1 by our selves after call this function.

wait(): add `if(p->isthread) continue` which means wait() function didnot deal with thread. 

fork(): add `np->isthread = 0` which means fork will create a process not a thread.

####syscall.c

argptr(): remove `(uint)i == 0` which means the pointer could be a null pointer. It's ok if you do not dereference it but just pass it as a parameter.

#### Makefile

Add kthread.o to the ULIB so it's a user library and can be used. Add testkthread.c.

#### other files

defs.h, sysproc.c, syscall.h, user.h, usys.S. Just the files need to be changed by adding system call. 

### Test case

The test case create three producer and two consumer and use lock to add and delete the things to see if multithread could get the right result.

For test this part, run the xv6 OS then input:
`testkthreads`



























































