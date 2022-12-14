/* server.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

//server socket
int serv_sock;
//camera client socket
int camera_client_sock = -1;

char buf[256];
//server address
struct sockaddr_in serv_addr;
//camera_client address
struct sockaddr_in camera_client_addr;
socklen_t camera_client_addr_size;

//서버에 connect되었다고 보내줄 message
char message[]="Success connecting";

FILE *file = NULL;


void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


//camera client를 accept하는 함수
void *accept_camera_client(void *status){

	int serv2_sock = dup(serv_sock);

	int *state = (int*)status;
	*state = 0;
	if(camera_client_sock < 0){
		camera_client_addr_size=sizeof(camera_client_addr);
		
		//camera client 연결
		camera_client_sock=accept(serv2_sock, (struct sockaddr*)&camera_client_addr,&camera_client_addr_size);
		if(camera_client_sock==-1)
			error_handling("accept() error");
	}

	
	int can_receive = 2;
	if(send(camera_client_sock, &can_receive, sizeof(can_receive),0) == -1){
		error_handling("send con_ack error");
	}
	int imagesize_will_come = 0;

	while(imagesize_will_come != 3){
		if(recv(camera_client_sock, &imagesize_will_come, sizeof(imagesize_will_come), 0) == -1){
			error_handling("image_signal receive error");
		}
	}
	
	size_t image_size;
	if(recv(camera_client_sock, &image_size, sizeof(image_size),0)==-1){
		error_handling("image_size receive error");
	}
	
	printf("image size : %d\n", image_size);

	int nbyte = 256;
	size_t filesize = 0, bufsize = 256;
	fflush(file);
	file = fopen("current.png", "wb");
	
	
	
	int image_recv_state = 0;

	int image_will_come = 0;

	while(image_will_come != 4){
		if(recv(camera_client_sock, &image_will_come, sizeof(image_will_come), 0) == -1){
			error_handling("image will come signal error");
		}
	}

	int need_input = 0;
	int send_data;
	while(filesize != image_size){
		send_data = recv(camera_client_sock, buf, bufsize, 0);
		printf("%d\n", send_data);
		if(image_size < filesize+nbyte){
			need_input = image_size-filesize;
			printf("%d\n", need_input);
			fwrite(buf, sizeof(char), need_input, file);
			filesize += need_input;
		}
		else{
			fwrite(buf, sizeof(char), nbyte, file);
			filesize += nbyte;
		}
	}
	filesize = 0;
	image_recv_state = 1;
	write(camera_client_sock, (void*)&image_recv_state, sizeof(int));
	
	printf("finished store camera data\n");

	fclose(file);
	close(serv2_sock);
	
	*state = 1;
	pthread_exit((void*)&status);
}

int main(int argc, char *argv[])
{
	
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
	pthread_t p_thread[2];
	int thr_id;
	int status = 0;

	camera_client_addr_size=sizeof(camera_client_addr);
		
	//camera client 연결
	camera_client_sock=accept(serv_sock, (struct sockaddr*)&camera_client_addr,&camera_client_addr_size);
	if(camera_client_sock==-1)
		error_handling("accept() error");
	
	while(1){
		thr_id = pthread_create(&p_thread[0], NULL, accept_camera_client, (void *)&status);

		if(thr_id < 0){
			perror("thread create error");
		}

		pthread_join(p_thread[0], (void**)&status);
	}

	close(camera_client_sock);	
	close(serv_sock);
	return 0;
}