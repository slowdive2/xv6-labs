// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include <stdint.h>

void freerange(void *pa_start, void *pa_end, int id);
void cpu_kfree(void *pa, int id);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
  uint64 count;
};

struct kmem kmem[NCPU];

void
kinit()
{
  for(int i = 0; i < NCPU; i++){
    initlock(&kmem[i].lock, "kmem");
  }

    char *start = end;
    uintptr_t chunk = ((uintptr_t)PHYSTOP - (uintptr_t)end) / NCPU;

    for(int i = 0; i < NCPU; i++) {
        char *rng_start = start + i * chunk; // todo : better naming convention
        char *rng_end = (i == NCPU - 1)
               ? (char*)PHYSTOP
               : start + (i + 1) * chunk; // messy ternary stuff so that i dont round PHYSTOP down by accident

        freerange(rng_start, rng_end, i);
    }
  // initialize 8 free lists
}

void
freerange(void *pa_start, void *pa_end, int id)
{

  if(id == -1){ // default behavior
    char *p;
    p = (char*)PGROUNDUP((uint64)pa_start);
    for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
      kfree(p);
  }
  else{ // free w/ fixed cpuid
    char *p;
    p = (char*)PGROUNDUP((uint64)pa_start);
    for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
      cpu_kfree(p, id);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)

void
cpu_kfree(void *pa, int id)
{

  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  kmem[id].count++;
  release(&kmem[id].lock);
}

void
kfree(void *pa)
{

  struct run *r;
  int id;

  push_off();
  id = cpuid();
  pop_off();

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  kmem[id].count++;   
  release(&kmem[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.

int
steal_memlist(int id){ 
  struct run *r;
  struct run *z;
  acquire(&kmem[id].lock);
  for(int i = 0; i < NCPU; i++){
    int j = (id + i + 1) % NCPU; // randomness
      if(j == id)
        continue;

      acquire(&kmem[j].lock);
      if(kmem[j].count > 1){
        uint64 stln = kmem[j].count;
        r = kmem[j].freelist;
        for(int k = 0; k < ((kmem[j].count / 2) - 1); k++){ // get middle node
          r = r->next; 
        }
        z = kmem[id].freelist;
        kmem[id].freelist = kmem[j].freelist;
        kmem[j].freelist = r->next; // freelist now points to middle node + 1
        r->next = z;
        kmem[id].count += stln / 2;
        kmem[j].count -= stln / 2;
        release(&kmem[j].lock);
        return 1;
      }
      release(&kmem[j].lock);
    }
    return 0;
}

void *
kalloc(void)
{
  int id;
  int stolen = 1;
  struct run *r;

  push_off();
  id = cpuid();
  pop_off();

  acquire(&kmem[id].lock);
  
    if(!kmem[id].freelist){
      release(&kmem[id].lock);
      stolen = steal_memlist(id);
      acquire(&kmem[id].lock);
    }
    if(stolen){
      r = kmem[id].freelist;
      kmem[id].freelist = r->next;
      kmem[id].count--;
      release(&kmem[id].lock); 
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
    }
    else
    return 0;
}


// problem: only one free list, so each CPU has to wait for it to be free when calling kalloc()
// solution : give each CPU its own list of freemem at kernel init - evenly distributed initially

// problem 2 : CPUs relinquish their lists at different speeds, therefore:
// they attempt to steal another CPUs element from their free list
// thus, CPUs 0-3 are checked most frequently by the CPUs attempting to steal, which means:
// CPUs 4-8 rarely get used as backup memory by other CPUs