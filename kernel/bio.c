// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 13
struct {
  struct spinlock locks[NBUCKET];
  struct spinlock evictionLock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf buckets[NBUCKET];
} bcache;
static int myhash(int x){
  return x%NBUCKET;
}

void move_to_bucket(struct buf *b, uint blockNo){
  int id = myhash(blockNo);
  b->next->prev = b->prev;
  b->prev->next = b->next;
  b->next = bcache.buckets[id].next;
  b->prev = &bcache.buckets[id];
  bcache.buckets[id].next->prev = b;
  bcache.buckets[id].next = b;
}
void
binit(void)
{
  struct buf *b;
  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.locks[i], "bcache");
  }
  initlock(&bcache.evictionLock, "bcache eviction");

  // Create linked list of buffers
  for(int i = 0; i < NBUCKET; i++){
    bcache.buckets[i].prev = &bcache.buckets[i];
    bcache.buckets[i].next = &bcache.buckets[i];
  }
  
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.buckets[0].next;
    b->prev = &bcache.buckets[0];
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[0].next->prev = b;
    bcache.buckets[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id = myhash(blockno);
  acquire(&bcache.locks[id]);

  // Is the block already cached?
  for(b = bcache.buckets[id].next; b != &bcache.buckets[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  release(&bcache.locks[id]);
  acquire(&bcache.evictionLock);
  acquire(&bcache.locks[id]);
  for(b = bcache.buckets[id].next; b != &bcache.buckets[id]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[id]);
      release(&bcache.evictionLock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  for(b = bcache.buf; b != bcache.buf + NBUF; b++){
    int eid = myhash(b->blockno);
    if(eid != id){
      acquire(&bcache.locks[eid]);
    }
    if(b->refcnt == 0) {
      if(eid != id){
        move_to_bucket(b, blockno);
        release(&bcache.locks[eid]);
      }
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.evictionLock);
      release(&bcache.locks[id]);
      acquiresleep(&b->lock);
      return b;
    }
    if(eid != id){
      release(&bcache.locks[eid]);
    }
  }release(&bcache.locks[id]);
  release(&bcache.evictionLock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int id = myhash(b->blockno);
  acquire(&bcache.locks[id]);
  b->refcnt--;
  release(&bcache.locks[id]);
}

void
bpin(struct buf *b) {
  int id = myhash(b->blockno);
  acquire(&bcache.locks[id]);
  b->refcnt++;
  release(&bcache.locks[id]);
}

void
bunpin(struct buf *b) {
  int id = myhash(b->blockno);
  acquire(&bcache.locks[id]);
  b->refcnt--;
  release(&bcache.locks[id]);
}


