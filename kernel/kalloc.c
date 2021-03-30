// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

uint krel[PHYSTOP / PGSIZE];

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem;

void kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.freelist = 0;
  memset(krel, 0, sizeof(uint) * (PHYSTOP / PGSIZE));
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  r = (struct run *)pa;

  acquire(&kmem.lock);
  if (kget(pa) > 0)
  {
    kadd(pa, -1);
  }
  if (kget(pa) == 0)
  {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);
    r->next = kmem.freelist;
    kmem.freelist = r;
  }
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
  {
    kadd((void *)r, 1);
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

// count free memory
void kcount(void)
{
  uint count = 0, sum = 0, free = 0;
  int i;
  struct run *r;

  for (i = (KERNBASE / PGSIZE); i < PHYSTOP / PGSIZE; i++)
  {
    if (krel[i] > 0)
      count++;
    sum += krel[i];
  }

  r = kmem.freelist;
  while (r != 0)
  {
    free++;
    r = r->next;
  }
  printf("  kcount count: %d sum: %d free: %d\n", count, sum, free);
}

void kadd(void *pa, int n)
{
  // acquire(&kmem.lock);
  krel[(uint64)pa / PGSIZE] += n;
  // release(&kmem.lock);
}

uint kget(void *pa)
{
  uint n;
  // acquire(&kmem.lock);
  n = krel[(uint64)pa / PGSIZE];
  // release(&kmem.lock);
  return n;
}
