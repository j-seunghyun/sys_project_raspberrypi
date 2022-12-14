/*
get server.c
위험수준 요청을 get해가는 서버
//GPIO 4번 - > button1
//GPIO 17번 -> button2
//GPIO 27번 -> button3
*/

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

//server socket
int serv_sock;
//led client socket
int led_client_sock = -1;

struct sockaddr_in serv_addr;
//led_client address
struct sockaddr_in led_client_addr;
socklen_t led_client_addr_size;



void error_handling(char *message)
{
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

  if(-1 == GPIOWrite(BUTTON1, 1))
    return(3);
  
  if(-1 == GPIOWrite(BUTTON2, 1))
    return(3);
  
  if(-1 == GPIOWrite(BUTTON3, 1))
    return(3);
}

//led client를 accept 하는 함수
void *accept_led_client(void *status){
  
  int fd;
  //log file에 시간을 출력해주기 위함
  time_t timer;
  struct tm *t;
  char buf[100];
  timer = time(NULL);
  t = localtime(&timer);

  int *state = (int*)status;
  *state = 0;

  int dangerous_level = -1;
  int buf_size;

  //버튼 1이 눌려있을때
  if(GPIORead(BUTTON1) == 0){
    printf("%d\n", GPIORead(BUTTON1));
    printf("%d\n", GPIORead(BUTTON2));
    printf("%d\n", GPIORead(BUTTON3));
    dangerous_level = 1;
    if(send(led_client_sock, &dangerous_level, sizeof(dangerous_level),0) == -1){
      error_handling("send dangerous_level error");
    }
  }
  //버튼 2가 눌려있을때
  else if(GPIORead(BUTTON2) == 0){
    printf("%d\n", GPIORead(BUTTON1));
    printf("%d\n", GPIORead(BUTTON2));
    printf("%d\n", GPIORead(BUTTON3));
    dangerous_level = 2;
    if(send(led_client_sock, &dangerous_level, sizeof(dangerous_level),0) == -1){
      error_handling("send dangerous_level error");
    }
  }
  //버튼 3가 눌려있을때
  else if(GPIORead(BUTTON3) == 0){
    printf("%d\n", GPIORead(BUTTON1));
    printf("%d\n", GPIORead(BUTTON2));
    printf("%d\n", GPIORead(BUTTON3));
    dangerous_level = 3;
    if(send(led_client_sock, &dangerous_level, sizeof(dangerous_level),0) == -1){
      error_handling("send dangerous_level error");
    }
  }
  else{
    *state = 1;
    pthread_exit((void*)&status);
  }

  buf_size = snprintf(buf, 100, "%04d-%02d-%02d %02d:%02d:%02d dangerous_level : %d", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, dangerous_level);

  fd = open("./log.txt", O_RDWR | O_CREAT,0777);
  if(fd == -1){
    perror("failed to open log.txt file");
  }
  lseek(fd, 0, SEEK_END);

  write(fd, buf, buf_size);
  write(fd, "\n", 1);

  close(fd);
  *state = 1;
  pthread_exit((void*)&status);
}


int main(int argc, char *argv[])
{

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

	if(argc!=2){
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	//서버 소켓 생성
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");
	
	//서버 소켓 세팅
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));
	
	//network connection 서버 소켓
	if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1 )
		error_handling("bind() error"); 
	
	//listen
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");

	//for thread
	pthread_t p_thread[1];
	int thr_id;
	int status = 0;

	led_client_addr_size=sizeof(led_client_addr);
		
	//led client 연결
	led_client_sock=accept(serv_sock, (struct sockaddr*)&led_client_addr,&led_client_addr_size);
	if(led_client_sock==-1)
		error_handling("accept() error");
	
	while(1){
		thr_id = pthread_create(&p_thread[0], NULL, accept_led_client, (void *)&status);

		if(thr_id < 0){
			perror("thread create error");
		}

		pthread_join(p_thread[0], (void**)&status);
    sleep(1);
	}

	close(led_client_sock);	
	close(serv_sock);
	return 0;
}