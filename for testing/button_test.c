//GPIO 4번 - > button1
//GPIO 17번 -> button2
//GPIO 27번 -> button3


#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define BUTTON1 4
#define BUTTON2 17
#define BUTTON3 27
#define VALUE_MAX 40
#define BUFFER_MAX 3
#define DIRECTION_MAX 35

static int GPIOExport(int pin){
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if(-1 == fd){
    fprintf(stderr, "Failed to open export for writing!\n");
    return(-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return(0);
}

static int GPIODirection(int pin , int dir){

  static const char s_direction_str[] = "in\0out";
  char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

  fd = open(path , O_WRONLY|O_CREAT, 0777);
  if(-1 == fd){
    fprintf(stderr, "Failed to open gpio direction for writing!\n");
    fprintf(stderr, "%d\n", pin);
    return(-1);
  }

   if(-1 == write(fd, &s_direction_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)){
    fprintf(stderr, "Failed to set direction!\n");
    close(fd);
    return(-1);
  }

  close(fd);
  return(0);
}


static int GPIORead(int pin){
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);
  if(-1 == fd){
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return(-1);
  }

  if(-1 == read(fd, value_str, 3)){
    fprintf(stderr, "Failed to read value!\n");
    close(fd);
    return(-1);
  }

  close(fd);

  return(atoi(value_str));
}

static int GPIOWrite(int pin, int value){

  static const char s_values_str[] = "01";

  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if(-1 == fd){
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return(-1);
  }

  if(1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)){
    fprintf(stderr, "Failed to write value!\n");
    close(fd);
    return(-1);
  }

  close(fd);
  return(0);
}

static int GPIOUnexport(int pin){
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if(-1 == fd){
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return(-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return(0);
}

//initialize all button and led
static int initialize(){

  if(-1 == GPIOWrite(BUTTON1, 1))
    return(3);
  
  if(-1 == GPIOWrite(BUTTON2, 1))
    return(3);
  
  if(-1 == GPIOWrite(BUTTON3, 1))
    return(3);
}


int main(int argc, char *argv[]){

  //initialize
  if(-1 == GPIOExport(BUTTON1)){
    return(1);
  }

  if(-1 == GPIODirection(BUTTON1, OUT))
    return(2);

  if(-1 == GPIOExport(BUTTON2))
    return(1);
  if(-1 == GPIODirection(BUTTON2, OUT))
    return(1);

  if(-1 == GPIOExport(BUTTON3))
    return(1);
  if(-1 == GPIODirection(BUTTON3, OUT))
    return(1);

  initialize();

  while(1){

    //버튼 1이 눌려있을때
    if(GPIORead(BUTTON1) == 0){
      printf("%d\n", GPIORead(BUTTON1));
      printf("%d\n", GPIORead(BUTTON2));
      printf("%d\n", GPIORead(BUTTON3));
    }
    //버튼 2가 눌려있을때
    else if(GPIORead(BUTTON2) == 0){
      printf("%d\n", GPIORead(BUTTON1));
      printf("%d\n", GPIORead(BUTTON2));
      printf("%d\n", GPIORead(BUTTON3));
    }
    //버튼 3가 눌려있을때
    else if(GPIORead(BUTTON3) == 0){
      printf("%d\n", GPIORead(BUTTON1));
      printf("%d\n", GPIORead(BUTTON2));
      printf("%d\n", GPIORead(BUTTON3));
    }

    sleep(1);
  }

  
}