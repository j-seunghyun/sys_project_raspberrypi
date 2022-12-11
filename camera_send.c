#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	char msg[2];
	char on[2] = "1";
	int str_len;
	int light = 0;

	if(argc !=3){
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock == -1){
		error_handling("socket() error");
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin.addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(connect(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) ==-1){
		error_handling("connect() error");
	}

	str_len = read(serv_sock. message, sizeof(message)-1);

	if(str_len == -1){
		error_handling("read() error");
	}
	printf("Message from server: %s \n", message);4

	//for jpg
	size_t fsize, nsize = 0;
	size_t fsize2;

	while(1)
	{
		int count = 0;

		system("raspistill -o hey.jpg");
		usleep(1000);
		file = fopen("hey.jpg", "rb");

		//파일 포인터 맨 끝으로 이동
		fseek(file, 0, SEEK_END);
		//file size측정
		fsize = ftell(file);
		
		//다시 file맨앞으로
		fseek(file, 0, SEEK_SET);
		while(nsize != fsize){
			//read file and insert to buffer
			int fpsize = fread(buf, 1, 256, file);
			nsize += fpsize;
			send(serv_sock, buf, fpsize, 0);
		}
		fclose(file);
		usleep(500);
	}
	return 0;
}