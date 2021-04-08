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

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

void kinit()
{
  char name[6] = "kmem0";
  for (int i = 0; i < NCPU; i++)
  {
    char lockname[6] = {0};
    strncpy(lockname, name, strlen(name));
    lockname[4] = i + 48;
    initlock(&kmem[i].lock, lockname);
  }
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
  int id;

  if (((uint64)pa % PGSIZE) != 0)
    panic("kfree0");
  if ((char *)pa < end)
    panic("kfree1");
  if ((uint64)pa >= PHYSTOP)
    panic("kfree2");
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  id = cpuid();
  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int id, rid;

  rid = cpuid();
  acquire(&kmem[rid].lock);
  r = kmem[rid].freelist;
  if (r)
    kmem[rid].freelist = r->next;
  else
  {
    // steal mem
    for (int i = 1; i < NCPU; i++)
    {
      id = rid + i;
      if (id > NCPU)
        id -= NCPU;

      acquire(&kmem[id].lock);
      r = kmem[id].freelist;
      if (r)
        kmem[id].freelist = r->next;
      release(&kmem[id].lock);
      if (r)
        break;
    }
  }
  release(&kmem[rid].lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}
