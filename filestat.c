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

int main(int argc, char *argv[]){
  if(argc != 2){
    printf(stdout, "input error, the format should be like: filestat path/filename"); 
  }
  struct stat mystat;
  stat(argv[1], &mystat);
  printf(stdout, "type: %d, size: %d, checksum: %d\n",mystat.type, mystat.size, (uint)mystat.checksum); 
}
