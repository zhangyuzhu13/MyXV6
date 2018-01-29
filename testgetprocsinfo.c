#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"
#include "procinfo.h"

int stdout = 1;

void 
getprocsinfotest(){
  struct procinfo info[64];
  int procnum = getprocsinfo(info);
  if(procnum < 0)
  {
    printf(stdout,"getprocsinfo test failed");
	exit();
  }
  else{
	printf(stdout, "amount of processes:%d\n", procnum);
	
	struct procinfo *in;
  	for(in = info; in < &info[procnum]; in++)
    {  	   
	  printf(stdout, "pid:%d, name:%s\n", in->pid, in->pname);
    }
	exit();
  }
}


int
main(int argc, char *argv[])
{
  printf(1, "testgetprocsinfo starting\n");
/*
  if(open("testgetprocsinfo.ran", 0) >= 0){
    printf(1, "already ran testgetprocsinfo -- rebuild fs.img\n");
    exit();
  }
  close(open("testgetprocsinfo.ran", O_CREATE));
  */
  getprocsinfotest();
  exit();
}
