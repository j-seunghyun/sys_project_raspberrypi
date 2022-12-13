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


pthread_t p_thread[1];

void error_handling(char *message){
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int my_system(const char *cmd){

	pid_t pid;
	int status;
	if(cmd == NULL) return 1;
	if((pid = fork()) <0 ){
		status = -1;
	}
	//자식 프로세스에서 raspistill
	else if(pid == 0){
		execl("/bin/sh", "sh", "-c", cmd, (char*)0);
		_exit(127);
	}
	//부모 프로세스
	else{
		while(waitpid(pid, &status, 0) < 0){
			if(errno != EINTR){
				status = -1;
				break;
			}
		}
	}
	return status;
}

void *cameraFilm(void *status){
	int camera_ok;
	int *state = (int*)status;
	*state = 0;

	printf("camera\n");
	printf("3\n");
	sleep(1);
	printf("2\n");
	sleep(1);
	printf("1\n");
	sleep(1);

	camera_ok = my_system("raspistill -o current.png -w 120 -h 100");
	printf("%d\n", camera_ok);
	
	if(camera_ok == -1){
		error_handling("camera error");
	}
	else{
		int *state = (int*)status;
		*state = 1;
	}
	pthread_exit((void*)&status);
}

int main(int argc, char *argv[])
{

  //for thread
	int thr_id;
	int status = 0;

  while(1)
	{
		//thread create
		thr_id = pthread_create(&p_thread[0], NULL, cameraFilm, (void*)&status);
		
		if(thr_id < 0){
			perror("thread create error");
		}

		//camera가 찍힌 다음에 이미지가 socket을 통해 전송될 수 있도록
		//thread_join으로 동기화 시킨다.
		pthread_join(p_thread[0], (void **)&status);

  }
}