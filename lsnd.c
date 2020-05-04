#include "types.h"
#include "stat.h"
#include "param.h"
#include "fs.h"
#include "user.h"

int execute(char * command, char** args){
    int pid;

    if((pid = fork()) == 0){
        exec(command, args);
        printf(1, "exec %s failed\n", command);
        exit();
    }
    else if(pid > 0){
        wait();
        return pid;
    }
    else{
        printf(1,"fork failed\n");
        exit();
    }
}

void print_filestat(){
  char * command;
  char *args[4];

  printf(1,"Printing filestat\n");
  args[0] = "/cat";
  args[1] = "proc/filestat";
  args[2] = 0;
  args[3] = 0;
  command = "/cat";
  execute(command,args);
}

void print_ideinfo(){
  char * command;
  char *args[4];

  printf(1,"Printing ideinfo\n");
  args[0] = "/cat";
  args[1] = "proc/ideinfo";
  args[2] = 0;
  args[3] = 0;
  command = "/cat";
  execute(command,args);
}

//fetch a field between c1 and c2, return the index of c2.
int get_field(char * dst, char *buf, char c1, char c2, int from){
	int i;
  for (i=0; i<10; i++){
  	dst[i] = 0; 
  }
	int s = indexof(buf,c1,from);
	int e = indexof(buf,c2,s+1);
  memmove(dst, buf+s+1, e-s-1);
  return e;
}

void print_inode(char * buf, char * path){
	char temp_buf[10];
	char type[6];


  int e = get_field(temp_buf, path, '/', 0, 0);

  e = get_field(temp_buf, buf, '\t', '\n', 0);
  int device = atoi(temp_buf);

  e = get_field(temp_buf, buf, '\t', '\n', e);
  int inum = atoi(temp_buf);

  e = get_field(temp_buf, buf, '\t', '\n', e);
  int valid = atoi(temp_buf);

  e = get_field(type, buf, '\t', '\n', e);

  e = get_field(temp_buf, buf, '\t', ',', e);
  int major = atoi(temp_buf);

  e = get_field(temp_buf, buf, ',', '\n', e);
  int minor = atoi(temp_buf);
  
  e = get_field(temp_buf, buf, '\n', '\t', e);
  e = get_field(temp_buf, buf, '\t', '\n', e);
  int hard_links = atoi(temp_buf);

  e = get_field(temp_buf, buf, '\t', '\n', e);
  int b_used = atoi(temp_buf);

 printf(1,"%d %d %d %s (%d,%d) %d %d\n",device, inum, valid, type, major, minor, hard_links ,b_used);
//<#device> <#inode> <is valid> <type> <(major,minor)> <hard links> <blocks used>
//printf(1, "%d %d %d %s %d %d %d\n",device, inum, valid, type, major_minor, hard_links ,b_used);

}

void openfd(char *path){
	char buf[512];
  memset(buf, 0, 512);
	int fd;
  if((fd = open(path, 0)) < 0){
    printf(2, "can't open %s\n", path);
    return;
  }
  int n;
  if((n = read(fd, buf, sizeof(buf))) > 0)
    print_inode(buf, path);
  if(n < 0)
    printf(2, "error reading\n");
  close(fd);
}

void lsnd(char *path){
  char buf[512], *p;
  int fd;
  struct dirent dir;
  if((fd = open(path, 0)) < 0){
    printf(2, "lsnd: can't open %s\n", path);
    return;
  }
  
  if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf))
    printf(1, "long path\n");
  strcpy(buf, path);
  p = buf+strlen(buf);
  *p++ = '/';
  while(read(fd, &dir, sizeof(dir)) == sizeof(dir)){
    if(dir.inum == 0){
      continue;
    }
    memmove(p, dir.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if ((dir.inum % 100 >= 10 && dir.inum % 100 <= 25 && dir.inum > 200)){
    	openfd(buf);
    }
    if ((strcmp(dir.name, "inodeinfo") == 0) || ((dir.inum % 100 == 0) && (*dir.name != '.'))){ // check for dirs
    	lsnd(buf);
    }
  }
  close(fd);
}

int main(int argc, char *argv[]){
  // print_filestat();
  // printf(1,"\n");
  // print_ideinfo();
  // printf(1,"\n");
  lsnd("proc");
  exit();
}
