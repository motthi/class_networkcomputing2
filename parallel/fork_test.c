#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void signal_handler(int sig);

int main() {
	int listen_sock, accept_sock, pid;
	socklen_t sin_siz;
	struct sockaddr_in addr;
	struct sockaddr_in serv, clt;
	unsigned short port;
	int yes = 1;

	printf("Now Setting...\n");
	signal(SIGCHLD, signal_handler);

	listen_sock			 = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family		 = AF_INET;
	addr.sin_port		 = htons(22629);
	addr.sin_addr.s_addr = INADDR_ANY;

	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));		  //TIME_WAIT状態でも再起動可能に設定
	if(bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {					  //通信元の情報を与える
		perror("bind");
		return 0;
	}
	if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	printf("Ready to Start\n");
	while(1) {
		accept_sock = accept(listen_sock, (struct sockaddr*)&clt, &sin_siz);
		pid			= fork();
		if(pid < 0) {
			perror("fork");
			exit(1);
		} else if(pid == 0) {
			close(listen_sock);
			write(accept_sock, "Hello\n", 7);
			exit(0);
		}
		close(accept_sock);
	}
}

void signal_handler(int sig) {
	int status, retval;
	do {
		retval = waitpid(-1, &status, WNOHANG);
	} while(retval > 0);
	signal(SIGCHLD, signal_handler);
}