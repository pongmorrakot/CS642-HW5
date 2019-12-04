#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>

/* Performs a dummy access
   to the address specified. */
void maccess(void *addr)
{
    asm volatile ("movq (%0), %%rax\n"
        :
        : "c" (addr)
        : "rax");
}

/* Flushes the cache block corresponding
   to the address from the entire cache
   hierarchy. */
void flush(void *addr)
{
    asm volatile("clflush 0(%0)\n"
        :
        : "c" (addr)
        :"rax");
}

/* Reads the Time Stamp Counter. */
uint64_t rdtsc() {
  uint64_t a, d;
  asm volatile ("mfence");
  asm volatile ("rdtsc" : "=a" (a), "=d" (d));
  a = (d<<32) | a;
  asm volatile ("mfence");
  return a;
}

/* combines rdtsc and maccess */
uint64_t measure_one_block_access_time(void* addr)
{
    uint64_t cycles;

    asm volatile("mov %1, %%r8\n\t"
            "lfence\n\t"
            "rdtsc\n\t"
            "mov %%eax, %%edi\n\t"
            "mov (%%r8), %%r8\n\t"
            "lfence\n\t"
            "rdtsc\n\t"
            "sub %%edi, %%eax\n\t"
    : "=a"(cycles) /*output*/
    : "r"(addr)
    : "r8", "edi");

    return cycles;
}

int main(int argc, char** argv)
{
    char array[4*1024]; // modern processors have 4KB ways in L1
    memset(array, -1, 4*1024*sizeof(char));
    unsigned long hit_time = 0, miss_time = 0;    
    
    for(int i = 0; i < 4*1024; i++){
        /* Calculate average hit time for a cache block */
        maccess(&array[i]);
        sched_yield();
        hit_time += measure_one_block_access_time(&array[i]);
    }
    for(int i = 0; i < 4*1024; i++){
        /* Calculate average miss time for a cache block */
        flush(&array[i]);
        sched_yield();
        miss_time += measure_one_block_access_time(&array[i]);
    }
    printf("%lu,%lu\n", hit_time/(4*1024), miss_time/(4*1024));
    return 0;
}
