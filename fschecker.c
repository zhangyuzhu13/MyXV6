#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"

int fsfd;
struct superblock sb;
//struct dinode ind;
void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, BSIZE) != BSIZE){
    perror("read");
    exit(1);
  }
}
void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}


// check 1: Each inode is either unallocated or one of the valid types (T_FILE, T_DIR, T_DEV)
int checkinodetype()//struct superblock *sb)
{
  struct dinode in;
  // for loop the inodes to find the bad inode
  for(int num = 0; num < sb.ninodes; num++) {
    rinode(num, &in);
    if(in.type != 0 && in.type != T_DIR && in.type != T_FILE && in.type != T_DEV) {
      return -1;
    }
  }
  return 1;
}
// check 2: For in-use inodes, each address that is used by inode is valid (points to a valid datablock address within the image). Note: must check indirect blocks too, when they are in use.
int checkaddrvalid()
{
  uint maxnum = sb.size - 1; // address should small than maxnum
  uint minnum = BBLOCK(sb.size, sb); // address should bigger than minnum
  struct dinode in;
  // for loop the inodes to check each inode address
  for(int i = 0; i < sb.ninodes; i++){
    rinode(i, &in);
    for(int j = 0; j < NDIRECT + 1; j++){
      if(in.addrs[j] != 0 && (in.addrs[j] < minnum || in.addrs[j] > maxnum)){
        return -1;
      }
    }
    // deal with the indirect file block to check the address
    if(in.addrs[NDIRECT] != 0){
      char buf[BSIZE];
      rsect(in.addrs[NDIRECT], buf);
      uint indiraddrs[NINDIRECT];
      memmove(&indiraddrs, buf, sizeof(indiraddrs));
      for(int j = 0; j < NINDIRECT; j++){
        if(indiraddrs[j] != 0 && (indiraddrs[j] < minnum || indiraddrs[j] > maxnum)){
          return -1;
        }
      }
    }
  }
  return 1;
}

// check 3: root directory exist and its inode number is 1.
int checkrootexist()
{
  struct dinode in;
  rinode(1, &in);
  if(in.type != T_DIR) return -1;
  return 1;
}
// get the inum of dirent with specific name
int getinum(uint inum, char *name)
{
  struct dinode in;
  rinode(inum, &in);
  struct dirent dirs[DPB];
  char buf[BSIZE];
  if(in.type != T_DIR)
    return 0;
  // find directory inode and to check the direct directory format
  for(uint j = 0; j < NDIRECT; j++) {
    if(in.addrs[j] == 0)
      continue;
    rsect(in.addrs[j], buf);
    memmove(&dirs, buf, sizeof(dirs));
    for(uint k = 0; k < DPB; k++){
      // find the name, stop searching.
      if(strncmp(name, dirs[k].name, DIRSIZ) == 0)
        return dirs[k].inum;
    }
  }
  // search the indirect part to find the name
  if(in.addrs[NDIRECT] != 0){
    uint addrs[NINDIRECT];
    rsect(in.addrs[NDIRECT], buf);
    memmove(&addrs, buf, sizeof(addrs));
    // read each address
    for(uint j = 0; j < NINDIRECT; j++){
      if(addrs[j] == 0)
        continue;
      rsect(addrs[j], buf);
      memmove(&dirs, buf, sizeof(dirs));
      // read each dirent to find the name match
      for(uint k = 0; k < DPB; k++){
        if(strncmp(name, dirs[k].name, DIRSIZ) == 0)
          return dirs[k].inum;
      }
    }
  }
  return -1;
}

// check 4: each directory contains . and ..
int checkdirformat(char *name)
{
  struct dinode in;
  // loop all inode to check all directory inode
  for(uint i = 0; i < sb.ninodes; i++){
    rinode(i, &in);
    if(in.type != T_DIR) 
      continue;
    if(getinum(i, name) == -1)
      return -1;
  }
  return 1; 
}
// check 5: Each .. entry in directory refers to the proper parent inode, and parent inode points back to it
int checkparent()
{
  struct dinode in; 
  uchar buf[BSIZE];
  struct dirent dirs[DPB];
  // loop all inode to check all directory inode
  for(uint i = 0; i < sb.ninodes; i++){
    rinode(i, &in);
    if(in.type != T_DIR) 
      continue;
    // for each dirent inum, there should be a ".." in it's inode's address's dirent and point back to his parent.
    for(uint j = 0; j < NDIRECT; j++) {
      if(in.addrs[j] == 0)
        continue;
      rsect(in.addrs[j], buf);
      memmove(&dirs, buf, sizeof(dirs));
      for(uint k = 0; k < DPB; k++){
        uint inum = getinum((uint)dirs[k].inum, "..");
        if(inum > 0 && inum != i){
          printf("%d, %d\n", inum, i);
          return -1;
        }
      }
    }
    // search the indirect part
    if(in.addrs[NDIRECT] != 0){
      uint addrs[NINDIRECT];
      rsect(in.addrs[NDIRECT], buf);
      memmove(&addrs, buf, sizeof(addrs));
      // read each address
      for(uint j = 0; j < NINDIRECT; j++){
        if(addrs[j] == 0)
          continue;
        rsect(addrs[j], buf);
        memmove(&dirs, buf, sizeof(dirs));
        // read each dirent to find the name match
        for(uint k = 0; k < DPB; k++){
          uint inum = getinum((uint)dirs[k].inum, "..");
          if(inum > 0 && inum != i){
            printf("%d, %d\n", inum, i);
            return -1;
          }
        }
      }
    }
  }
  return 1;  
}
// check if the block num used in inode mark in bitmap
int getbit(uint addr)
{
  uchar buf[BSIZE];
  uint bnum = BBLOCK(addr, sb);
  rsect(bnum, buf);
  uint index = (addr % BPB) / 8; // bitmap block number
  uint shift = addr % 8; // addr shift bit
  // the bit in bitmap is marked return 1, not mark in bitmap return -1
  if((buf[index] >> shift) % 2 == 1){
    return 1;
  }
  else{
    printf("%d\n", addr);
    return -1;
  }
}

// check 6: For in-use inodes, each address in use is also marked in use in the bitmap


int checkbitmap(uint addresses[])
{
  struct dinode in;
  uchar buf[BSIZE];
  for(uint i = 0; i < sb.ninodes; i++){
    rinode(i, &in);
    if(in.type == 0)
      continue;
    // find directory inode and to check the direct directory format
    for(uint j = 0; j < NDIRECT; j++) {
      if(in.addrs[j] == 0)
        continue;
      addresses[in.addrs[j]] += 1;
      if(getbit(in.addrs[j]) != 1)//{printf("%d, ", in.addrs[j]);}
        return -1;
    }
    // search the indirect part to find the name
    if(in.addrs[NDIRECT] != 0){
      addresses[in.addrs[NDIRECT]] += 1; // the indirect block is also a data block which need to be record.
      uint addrs[NINDIRECT];
      rsect(in.addrs[NDIRECT], buf);
      memmove(&addrs, buf, sizeof(addrs));
      // read each address
      for(uint j = 0; j < NINDIRECT; j++){
        if(addrs[j] == 0)
          continue;
        addresses[addrs[j]] += 1;
        if(getbit(addrs[j]) != 1)//{printf("%d, ", addrs[j]);}
          return -1;        
      }
    }
  }
  return 1;
}

//check 7: For blocks marked in-use in bitmap, actually is in-use in an inode or indirect block somewhere
int checkblockuse(uint addresses[])
{
  uchar buf[BSIZE];
  // because only one block is bit map, just copy one into buf and change 1 to 0 if checked, to see if there is any 1 left in bitmap, this is eror 7
  rsect(sb.bmapstart, buf);
  uint start = sb.bmapstart + 1;
  for(uint i = start; i < sb.size; i++){
    uint index = i / 8;
    uint shift = i % 8;
    if((buf[index] >> shift) % 2 == 1){
      if(addresses[i] == 0){
        printf("%d\n", i);
        //return -1;
      }
    }
  }
  return 1;
}

//check 8: For in-use inodes, any address in use is only used once
int checkusetime(uint addresses[])
{
  for(uint i = 0; i < sb.size; i++){
    if(addresses[i] > 1){
      return -1;
    }
  }
  return 1;
}

//check 9: For inodes marked used in inode table, must be referred to in at least one directory.
int checkinoderef(uint inodes[])
{
  struct dinode in;
  for(uint i = 0; i < sb.ninodes; i++){
    rinode(i, &in);
    if(in.type != T_DIR) 
      continue;
    struct dirent dirs[DPB];
    char buf[BSIZE];
    // find directory inode and to check the direct directory format
    for(uint j = 0; j < NDIRECT; j++) {
      if(in.addrs[j] == 0)
        continue;
      rsect(in.addrs[j], buf);
      memmove(&dirs, buf, sizeof(dirs));
      for(uint k = 0; k < DPB; k++){
        // count the directory inode which be linked
        if(dirs[k].inum != 0 && strncmp("..", dirs[k].name, DIRSIZ) != 0 && strncmp(".", dirs[k].name, DIRSIZ) != 0){
          inodes[dirs[k].inum]++;
        }
      }
    }
    // search the indirect part to find the name
    if(in.addrs[NDIRECT] != 0){
      uint addrs[NINDIRECT];
      rsect(in.addrs[NDIRECT], buf);
      memmove(&addrs, buf, sizeof(addrs));
      // read each address
      for(uint j = 0; j < NINDIRECT; j++){
        if(addrs[j] == 0)
          continue;
        rsect(addrs[j], buf);
        memmove(&dirs, buf, sizeof(dirs));
        // read each dirent to find the name match
        for(uint k = 0; k < DPB; k++){
          // count the directory inode which be linked
          if(dirs[k].inum != 0 && strncmp("..", dirs[k].name, DIRSIZ) != 0 && strncmp(".", dirs[k].name, DIRSIZ) != 0){
            inodes[dirs[k].inum]++;
          }
        }
      }
    }
  }
  // inum 1 is the root directory so no need to check refernece to root.
  for(uint i = 2; i < sb.ninodes; i++){
    rinode(i, &in);
    if(in.type != 0 && inodes[i] == 0){
      printf("%d, %d\n", inodes[i], i);
      return -1;
    }
  }
  return 1;
}
//check 10: For inode numbers referred to in a valid directory, actually marked in use in inode table
int checkinodemark(uint inodes[])
{
  struct dinode in;
  for(uint i = 0; i < sb.ninodes; i++){
    if(inodes[i] > 0) {
      rinode(i, &in);
      if(in.type == 0){
        printf("%d, %d\n", i, inodes[i]);
        return -1;
      }
    }
  }
  return 1;
}

// check 11: Reference counts (number of links) for regular files match the number of times file is referred to in directories (i.e., hard links work correctly).
int checklinks(uint inodes[])
{ 
  struct dinode in;
  for(uint i = 0; i < sb.ninodes; i++){
    rinode(i, &in);
    if(in.type != T_FILE)
      continue;
    if(in.nlink != inodes[i]){
      printf("%d, %d, %d\n", i, inodes[i], in.nlink);
      return -1;
    }
  }
  return 1;
}

// check 12: No extra links allowed for directories (each directory only appears in one other directory)
int checkdirref(uint inodes[])
{
  struct dinode in;
  for(uint i = 0; i < sb.ninodes; i++){
    rinode(i, &in);
    if(in.type != T_DIR)
      continue;
    if(inodes[i] > 1){
      printf("%d, %d, %d\n", i, inodes[i], in.nlink);
      return -1;
    }
  }
  return 1;
}
int main(int argc, char *argv[]) {
	
  // check if filesystem image is provided
  if(argc < 2){
    fprintf(stderr, "ERROR: image not found. Usage: fschecker fs.img\n");
    return 1;
  } 
	
  // Open file system image
  fsfd = open(argv[1], O_RDONLY);
 
  if (fsfd < 0) { //check if file is present
    perror(argv[1]);
    return 1;
  }
  uchar buf[BSIZE];
  // read the superblock into buffer
  rsect(SUPERBLOCK, buf);
  memmove(&sb, buf, sizeof(sb));
  printf("sb.size: %d, sb.nblocks: %d, sb.ninodes:%d, sb.nlog: %d, sb.logstart: %d, sb.inodestart: %d, sb.bmapstart: %d\n",sb.size, sb.nblocks, sb.ninodes, sb.nlog, sb.logstart, sb.inodestart, sb.bmapstart); 
  rsect(sb.bmapstart, &buf);
  
  if(checkinodetype() == -1) {
    close(fsfd);
    fprintf(stderr, "ERROR: bad inode");
    return 1;
  }
  if(checkaddrvalid() == -1) {
    close(fsfd);
    fprintf(stderr, "ERROR: bad address in inode");
    return 1;
  }
  if(checkrootexist() == -1) {
    close(fsfd);
    fprintf(stderr, "ERROR: root directory does not exist");
    return 1;
  }
  if(checkdirformat(".") == -1 || checkdirformat("..") == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: directory not properly formatted");
    return 1;
  }
  if(checkparent() == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: parent directory mismatch");
    return 1;
  }
  uint addresses[sb.size];
  for(int i = 0; i < sb.size; i++){
    addresses[i] = 0;
  }
  if(checkbitmap(addresses) == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.");
    return 1;
  }
  if(checkblockuse(addresses) == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use");
    return 1;
  }
  if(checkusetime(addresses) == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: address used more than once");
    return 1;
  }
  uint inodes[sb.ninodes];
  for(uint i = 0; i < sb.ninodes; i++){
    inodes[i] = 0;
  }
  if(checkinoderef(inodes) == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: inode marked use but not found in a directory.");
    return 1;
  } 
  if(checkinodemark(inodes) == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: inode referred to in directory but marked free");
    return 1;
  }
  if(checklinks(inodes) == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: bad reference count for file");
    return 1;
  }
  if(checkdirref(inodes) == -1){
    close(fsfd);
    fprintf(stderr, "ERROR: directory appears more than once in file system");
    return 1;
  }
  printf("This file system is consistent!\n"); 
  return 0;
}
