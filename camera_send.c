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
	//자식 프로세스에서 raspistill명령 실행됨
	else if(pid == 0){
		execl("/bin/sh", "sh", "-c", cmd, (char*)0);
		_exit(127);
	}
	//부모 프로세스는 자식 프로세스가 끝날때까지 wait
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

void cameraFilm(){
	int camera_ok;

	printf("camera\n");
	printf("3\n");
	sleep(1);
	printf("2\n");
	sleep(1);
	printf("1\n");
	sleep(1);

	camera_ok = my_system("raspistill -o current.png -w 800 -h 600 -vf");
	printf("%d\n", camera_ok);
	
	if(camera_ok == -1){
		error_handling("camera error");
	}
}

int main(int argc, char *argv[])
{
	int serv_sock,fd;
	struct sockaddr_in serv_addr;
	int str_len, len;
	char message[30], buf[256];
	FILE* file = NULL;
	int camera_fd = 0;

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

	//for jpg
	size_t fsize = 0;
	size_t nsize = 0;
	size_t fsize2;
	
	//for thread
	int thr_id;
	int status = 0;

	while(1)
	{
		cameraFilm();

		//이 부분 open 으로 변경 필요할 거 같다.
		//camera_fd = open("hey.jpg", O_RDONLY)
		file = fopen("current.png", "rb");

		if(file == NULL){
			error_handling("file read error");
		}

		//파일 포인터 맨 끝으로 이동
		fseek(file, 0, SEEK_END);
		//file size측정
		fsize = ftell(file);
		//다시 file맨앞으로
		fseek(file, 0, SEEK_SET);

		printf("%d\n", fsize);

		
		int can_send = 0;
		while(can_send != 2){
			if(recv(serv_sock, &can_send, sizeof(can_send),0) == -1){
				error_handling("fail to receive current server_state");
			}
		}
		printf("%d\n", can_send);

		int will_send_image_size = 3;

		if(send(serv_sock, &will_send_image_size, sizeof(will_send_image_size), 0) == -1){
			error_handling("fail to send will_send_image signal");
		}
	
		//먼저 image size를 보내준다.
		if(send(serv_sock,&fsize,sizeof(fsize),0) == -1){
			error_handling("fail to send image size");
		}

		
		int will_send_image = 4;
		if(send(serv_sock, &will_send_image, sizeof(will_send_image), 0) == -1){
			error_handling("fail to send will send image signal");
		}

		int fpsize;
		//파일을 읽고 buffer에 담는다.
		while(nsize < fsize){
			//read file and insert to buffer
			fpsize = fread(buf, 1, 256, file);
			nsize += fpsize;
			if(send(serv_sock, buf, fpsize, 0)==-1){
				error_handling("fail to send");
			}
		}

		memset(&buf, 0, sizeof(buf));
		nsize = 0;
		//camera의 코드로 넘어가도 되는지 확인
		int read_camera = 0;
		while(read_camera != 1){
			if(read(serv_sock,(void*)&read_camera, sizeof(int)) ==-1){
				error_handling("read error");
			}
		}
		fclose(file);
		sleep(10);
	}
	close(serv_sock);
	return 0;
}