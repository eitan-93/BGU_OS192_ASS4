#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

int ips_index = 0;
struct inodeinfo ips[NINODE];
int number_of_inodes = 0;

int ideinfo(char *buf){
  struct Ideinfo* ide = get_ideinfo();
  
  add_to_buf(buf, "Waiting operations: ");
	add_num_to_buf(buf, ide->wait_op);
	add_to_buf(buf,"\nRead waiting operations: ");
  add_num_to_buf(buf, ide->wait_read_op);
  add_to_buf(buf,"\nWrite waiting operations: ");
  add_num_to_buf(buf, ide->wait_write_op);
  add_to_buf(buf,"\nWorking blocks: \n");
  
  for(int i = 0; i<device_num_size ; i++){
    add_num_to_buf(buf, ide->device_num[i]);
    add_to_buf(buf,",");
    add_num_to_buf(buf, ide->b_num[i]);
    add_to_buf(buf,"; ");
  }
  add_to_buf(buf,"\n");
	return strlen(buf);
}

int file_stat(char *buf){
  struct Filestat* fstat = get_filestat();

	add_to_buf(buf, "Free fds: ");
  add_num_to_buf(buf, fstat->free_fds);
  add_to_buf(buf, "\nUnique inode fds: ");
  add_num_to_buf(buf, fstat->unique_inodes_fds);
  add_to_buf(buf, "\nWriteable fds: ");
  add_num_to_buf(buf, fstat->writable_fds);
  add_to_buf(buf, "\nReadable fds: ");
  add_num_to_buf(buf, fstat->readable_fds);
  add_to_buf(buf, "\nRefs per fds: ");
  add_num_to_buf(buf, fstat->refs_per_fds);
  add_to_buf(buf,"\n");
  return strlen(buf);
}

int make_pid_dirs(char *buf){
	short slot = buf[0];
  buf[0] = 0;
	int pid = get_slot_pid(slot);
  char dirP[9] = {0};
  add_to_buf(dirP, "/proc/");
	add_num_to_buf(dirP, pid);
	add_dirent_to_buf(buf,".", namei(dirP)->inum, 0);
	add_dirent_to_buf(buf,"..", namei("/proc")->inum, 1); // cat proc/2/name
  add_dirent_to_buf(buf,"status", number_of_inodes+1+(slot+1)*100, 2);
  add_dirent_to_buf(buf,"name", number_of_inodes+2+(slot+1)*100, 3);
	return sizeof(struct dirent)*4;
}

int set_proc_dir(char *buf){
	add_dirent_to_buf(buf,".", namei("/proc")->inum, 0);
	add_dirent_to_buf(buf,"..", namei("")->inum, 1);
	add_dirent_to_buf(buf,"ideinfo", number_of_inodes+1, 2);
  add_dirent_to_buf(buf,"filestat", number_of_inodes+2, 3);
  add_dirent_to_buf(buf,"inodeinfo", number_of_inodes+3, 4);
  
  int pids[NPROC] = {0};
  get_empty_pids(pids);
  int i, j = 5;
  char str_num[10] = {0};
  for (i = 0; i<NPROC; i++){
  	if (pids[i] != 0){
  		for (int k = 0; k<10; k++) // this might not be needed.
        str_num[k] = 0;
  		
	    itoa(str_num, pids[i]);
	    add_dirent_to_buf(buf,str_num, number_of_inodes+(i+1)*100, j);
	    j++;
  	}
  }
 	return sizeof(struct dirent)*j;
}


int set_inodeinfo_dir(char *buf){
	short slot = buf[0];
  buf[0] = 0;

  add_dirent_to_buf(buf,"..", namei("/proc")->inum, 0);
  add_dirent_to_buf(buf,".", namei("/proc/inodeinfo")->inum, 1);

	int i;
	int j = 2;
	char str_num[10] = {0};
	for (i = 0; i < ips_size; i++){
		if (&ips[i] > 0){
	    itoa(str_num, ips[i].icache_idx);
      
 		  add_dirent_to_buf(buf,str_num, number_of_inodes+10+(slot+1)*100+i , j);
		  j++;
		}
	}
	return sizeof(struct dirent)*j;
}

int inodeinfo(char *buf){
get_inodeinfo(ips);
struct inodeinfo *gip = &ips[ips_index];
   
    add_to_buf(buf,"Device:\t");
    add_num_to_buf(buf, gip->ip->dev);
    add_to_buf(buf,"\nInode number:\t");
    add_num_to_buf(buf, gip->ip->inum);
    add_to_buf(buf,"\nis valid:\t");
    add_num_to_buf(buf, gip->ip->valid);
    add_to_buf(buf,"\ntype:\t");
    switch (gip->ip->type){
      case T_DIR:
        add_to_buf(buf,"DIR");
        break;
      case T_FILE:
        add_to_buf(buf,"FILE");
        break;
      case T_DEV:
        add_to_buf(buf,"DEV");
    }
    add_to_buf(buf,"\nmajor minor:\t");
    add_num_to_buf(buf, gip->ip->major);
    add_to_buf(buf,",");
    add_num_to_buf(buf, gip->ip->minor);
    add_to_buf(buf,"\nhard links:\t");
    add_num_to_buf(buf, gip->ip->nlink);
    add_to_buf(buf,"\nblocks used:\t");
    if(gip->ip->type == T_DEV)
      add_num_to_buf(buf, 0);
    else
      add_num_to_buf(buf, gip->blocks_in_use);
    add_to_buf(buf,"\n");

	return strlen(buf);
}

void add_to_buf(char *buf, char * string){ memmove(buf + strlen(buf), string,strlen(string)); }

void add_num_to_buf(char *buf, int num){
  char char_num[10];
  memset(char_num, 0, 10);
  itoa(char_num, num);
  add_to_buf(buf, char_num);
}

void add_dirent_to_buf(char *buff, char * dirName, int inum, int off){
  struct dirent dir;
	dir.inum = inum;
	memmove(&dir.name, dirName, strlen(dirName)+1);
  memmove(buff + off*sizeof(dir) , (void*)&dir, sizeof(dir));
}



int
procfsisdir(struct inode *ip) {
  if (!(ip->type == T_DEV && ip->major == PROCFS))
		return 0;

  struct superblock superb;
  
  if (number_of_inodes == 0){ // if the # of inodes wasn't set yet, obtain it from the superblock.
  	readsb(ip->dev, &superb); // read from the right superblock (block 1) and init superb
  	number_of_inodes = superb.ninodes;
  }
  int inum = ip->inum;
  if (inum == (number_of_inodes+1) || inum == (number_of_inodes+2))
      return 0; 
  // filestat and ideinfo files
  
  return (inum < number_of_inodes  || inum % 100 == 0 || inum == (number_of_inodes+3));
}

void 
procfsiread(struct inode* dp, struct inode *ip) {
  ip->valid = 1; 
	ip->major = PROCFS;
	ip->type = T_DEV;
  //ip->nlink = 1; might help with "incorrect blocknum"
}


int procfsread(struct inode *ip, char *dst, int off, int n) {
	int size;
  struct superblock sb;
  char buf[1056]; //longest data is (64 + 2) dirents * 16 bytes. just in case. we only need sizeof 3 dirs.
  memset(buf, 0, 1056);
  int file_num = 0;

  if (number_of_inodes == 0){
  	readsb(ip->dev, &sb);
  	number_of_inodes = sb.ninodes;
  }

	short slot = 0;
	if (ip->inum >= number_of_inodes+100){
		slot = (ip->inum - number_of_inodes)/100 - 1;
		buf[0] = slot;
	}
  //cprintf("in procfsread : ip->inum is %d\n",ip->inum);
	//size = set_procfs(ip,buf);
  file_num = (ip->inum % 100) -10;


    if (ip->inum == namei("/proc")->inum /*< number_of_inodes */){

      size = set_proc_dir(buf);      // proc directory

       goto fill_file;
     }
    if (ip->inum == (number_of_inodes+1)){			
      size = ideinfo(buf);           // ideinfo file
      goto fill_file;
    }
    if (ip->inum == (number_of_inodes+2)){    	
      size = file_stat(buf);         // filestat file
      goto fill_file;
    }
    if (ip->inum == (number_of_inodes+3)){
      get_inodeinfo(ips);
      size = set_inodeinfo_dir(buf); // inodefolder

      goto fill_file;
    }
   
    switch(ip->inum % 100) {
      case 0:
        size = make_pid_dirs(buf);
        break;
      case 1:
        size = status(buf);
        break;
      case 2:
        size = name(buf);
        break;
      default:
      if (file_num >= 0 && file_num <= number_of_inodes){

        // get_inodeinfo(ips);
        //   for(int i = 0;i < NINODE ; i++){
        //    if(&ips[i] !=0 && ips[i].ip->inum < number_of_inodes)
        //     ips_index = i;
        //     size += inodeinfo(buf);
        //   } 
        
       
        //cprintf("filenum %d\n",file_num);
          ips_index = file_num;
          size = inodeinfo(buf);
      }
      else {
        cprintf("bad inum\n");
        return 0;
      }
    }
  fill_file:
	memmove(dst, buf+off, n);
  
	return n < size-off ? n : size-off ;
}

int
procfswrite(struct inode *ip, char *buf, int n){
  // panic("panic : procfs is a read-only file system");
  return -1; // was 0, changed to pass some tests
}

void
procfsinit(void)
{
  devsw[PROCFS].isdir = procfsisdir;
  devsw[PROCFS].iread = procfsiread;
  devsw[PROCFS].write = procfswrite;
  devsw[PROCFS].read = procfsread;
}