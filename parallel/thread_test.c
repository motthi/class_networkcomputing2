#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void* send_recv_thread(void* arg);

int main() {
	int listen_sock, accept_sock;
	socklen_t sin_siz;
	struct sockaddr_in addr;
	struct sockaddr_in clt;
	int yes = 1;
	pthread_t tid;

	printf("Now Setting...\n");

	listen_sock			 = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family		 = AF_INET;
	addr.sin_port		 = htons(22629);
	addr.sin_addr.s_addr = INADDR_ANY;

	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));		  //TIME_WAIT状態でも再起動可能に設定
	if(bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {					  //通信元の情報を与える
		perror("bind");
		return 0;
	}
	if(listen_sock < 0) {
		perror("socket");
		exit(1);
	}
	listen(listen_sock, 5);

	printf("Ready to Start\n");
	while(1) {
		sin_siz		= sizeof(clt);
		accept_sock = accept(listen_sock, (struct sockaddr*)&clt, &sin_siz);
		if(pthread_create(&tid, NULL, &send_recv_thread, (void*)accept_sock) != 0) {
			perror("pthread_create");
		}
		pthread_detach(&tid);
	}
}

void* send_recv_thread(void* arg) {
	pthread_detach(pthread_self());
	int sock;
	sock = (int)arg;
	for(;;) {
		write(sock, "Hello\n", 7);
		//printf("accepted connection from %s, port=%d\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));
	}
	close(sock);
	pthread_exit((void*)0);
	return (NULL);
}