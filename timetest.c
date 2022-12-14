#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>

int main(){

  time_t timer;
  struct tm *t;
  timer = time(NULL);
  t = localtime(&timer);
  char buf[100];

  int fd;
  int buf_size;
  int dangerous_level;

  while (1){

    dangerous_level = 3;

    buf_size = snprintf(buf, 100, "%04d-%02d-%02d %02d:%02d:%02d dangerous_level : %d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, dangerous_level);

    fd = open("./log.txt", O_RDWR | O_CREAT , 0777);
    lseek(fd, 0, SEEK_END);

    write(fd, buf, buf_size);
    write(fd, "\n", 1);

    sleep(5);
  }






  return 0;
}