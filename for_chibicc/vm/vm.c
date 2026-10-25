#include "vm.h"

uint64_t r[1024] = {0};
uint64_t vstack[1024];
int vsp = 0;
uint64_t rax = 0;