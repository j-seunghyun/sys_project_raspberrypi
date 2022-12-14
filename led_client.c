/*
led_client.c
서버로부터 위험수준 레벨을 받아
위험수준 레벨에 맞게
led 와 부저를 켜줌

led 빨강 -> gpio 4번 (위험수준 레벨 3)
led 노랑 -> gpio 17번 (위험수준 레벨 2)
led 파랑 -> gpio 27번 (위험수준 레벨 1)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define LED_RED 4
#define LED_YEL 17
#define LED_BLUE 27
#define VALUE_MAX 40
#define BUFFER_MAX 3
#define DIRECTION_MAX 35

void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

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

  if(-1 == GPIOWrite(LED_RED, 0))
    return(3);
  
  if(-1 == GPIOWrite(LED_YEL, 0))
    return(3);

  if(-1 == GPIOWrite(LED_BLUE, 0))
    return(3);
}


int main(int argc, char *argv[]){

  int serv_sock,fd;
	struct sockaddr_in serv_addr;
	int str_len, len;
  int dangerous_level = -1;

  //gpio initialize
  if(-1 == GPIOExport(LED_RED))
    return(1);
      
  if(-1 == GPIODirection(LED_RED, OUT))
    return(2);

  if(-1 == GPIOExport(LED_YEL))
    return(1);
  
  if(-1 == GPIODirection(LED_YEL, OUT))
    return(2);

  if(-1 == GPIOExport(LED_BLUE))
    return(1);
  
  if(-1 == GPIODirection(LED_BLUE, OUT))
    return(2);

  initialize();

  if(argc !=3){
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	//socket 생성
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	if(serv_sock == -1){
		error_handling("socket() error");
	}

	//socket 세팅 -> internet domain 사용
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(connect(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ==-1){
		error_handling("connect() error");
	}

  while(1){

    if(recv(serv_sock, &dangerous_level, sizeof(dangerous_level), 0)== -1){
      error_handling("fail to receive dangerous level");
    }
    printf("%d\n", dangerous_level);

    switch(dangerous_level){
      //case 1은 위험수준 레벨1 -> blue light on
      //blue light만 on 시키고 나머지는 다 off
      case 1 :
        if(-1 == GPIOWrite(LED_BLUE, 1))
          return(3);
        if(-1 == GPIOWrite(LED_YEL, 0))
          return(3);
        if(-1 == GPIOWrite(LED_RED, 0))
          return(3);

        break;
        //case 2는 위험 수준 레벨 2-> yellow light만 on
      case 2 :
        if(-1 == GPIOWrite(LED_YEL, 1))
          return(3);
        if(-1 == GPIOWrite(LED_BLUE, 0))
          return(3);
        if(-1 == GPIOWrite(LED_RED, 0))
          return(3);
        
        break;
        //case 3는 위험 수준 레벨 3-> red light 만 on
      case 3 :
        if(-1 == GPIOWrite(LED_RED, 1))
          return(3);
        if(-1 == GPIOWrite(LED_BLUE, 0))
          return(3);
        if(-1 == GPIOWrite(LED_YEL, 0))
          return(3);
        break;

      default :
        break;
    }

    sleep(1);
  }

  close(serv_sock);
  return 0;
}