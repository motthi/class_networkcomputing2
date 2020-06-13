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
	struct sockaddr_in serv, clt;
	unsigned short port;

	signal(SIGCHLD, signal_handler);

	if((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

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