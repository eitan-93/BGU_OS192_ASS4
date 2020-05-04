//
// File descriptors
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

int different_inode_numbers[NFILE];

struct Filestat* get_filestat(){
  // cprintf("in get_filestat 1 %d\n",NINODE);
  // for(int i = 0; i <NFILE; i++)
  //   different_inode_numbers[i] = 1;
  //cprintf("in get_filestat 2 \n");
  Filestat.free_fds = 0;
  Filestat.unique_inodes_fds = 0;
  Filestat.writable_fds = 0;
  Filestat.readable_fds = 0;
  Filestat.refs_per_fds = 0;

  int num_used_fds = 0;
  int num_refs = 0;

  struct file *f;
  acquire(&ftable.lock);
  //cprintf("in get_filestat 3 \n");
 //int debug_c = 0;
 int different_inode_numbers_index = 0;
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f == 0 || !(f->ip->inum > 0) /*&& f->ip->inum < NFILE)*/)
      continue;

    //cprintf("in get_filestat 4  %d\n",debug_c++);
    if(f->ref == 0 /*&& f->type == FD_NONE*/)
      Filestat.free_fds++;
    //cprintf("in get_filestat before %d\n",f->ip->inum);

    int tmp_flag = 0;
    for(int j = 0; j < NFILE; j++){
     if(different_inode_numbers[j] == f->ip->inum){
          tmp_flag = 1;
          Filestat.unique_inodes_fds--;
          break; 
        } 
    }
      if(!tmp_flag){
        different_inode_numbers[different_inode_numbers_index++] = f->ip->inum;
      }
       Filestat.unique_inodes_fds++;

    if (f->writable) 
      Filestat.writable_fds++;
    
    if (f->readable) 
      Filestat.readable_fds++;
    
    if(f->type != FD_NONE /*f->ref > 0*/)
      num_used_fds++;

    num_refs += f->ref;

  }
  //cprintf("in get_filestat 6 \n");
  // Filestat.unique_inodes_fds = 100 - Filestat.free_fds;//might be a mistake, try what you wrote 
  Filestat.refs_per_fds = num_refs/num_used_fds;
  release(&ftable.lock);
  return &Filestat;
}

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE)
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
  if(f->type == FD_INODE){
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}

// Read from file f.
int
fileread(struct file *f, char *addr, int n)
{
  int r;

  if(f->readable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return piperead(f->pipe, addr, n);
  if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
    return r;
  }
  panic("fileread");
}

//PAGEBREAK!
// Write to file f.
int
filewrite(struct file *f, char *addr, int n)
{
  int r;

  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return pipewrite(f->pipe, addr, n);
  if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    return i == n ? n : -1;
  }
  panic("filewrite");
}

