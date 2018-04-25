#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"
#include "pstat.h"

int stdout = 1;

void 
ps(){
  struct pstat infos;
  struct pstat *in = &infos;
  int procnum = getpinfo(in);
  if(procnum < 0)
  {
    printf(stdout,"getprocsinfo test failed");
    exit();
  }
  else{
    printf(stdout, "amount of processes:%d\n", procnum);
    int index;
    for(index = 0; index < procnum; index++)
    {  	   
      printf(stdout, "pid:%d, running time in priority 1:%d, running time in priority 2: %d\n", in->pid[index], in->lticks[index], in->hticks[index]);
    }
  }
}
int
main(int argc, char *argv[])
{
  printf(1, "testgetpinfo starting\n");
  /*
  if(open("testgetprocsinfo.ran", 0) >= 0){
    printf(1, "already ran testgetprocsinfo -- rebuild fs.img\n");
    exit();
  }
  close(open("testgetprocsinfo.ran", O_CREATE));
  */
  ps();
  exit();
}
