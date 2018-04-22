#define NUM 64 
// all process states
struct pstat {
  int pids[NUM];
  int pri1_rtms[NUM];  // processes running time in priority 1
  int pri2_rtms[NUM];  // processes running time in priority 2
};
