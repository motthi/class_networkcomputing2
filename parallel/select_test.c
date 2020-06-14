#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void* send_recv_thread(void* arg);

int main() {
	int listen_sock, accept_sock;
	socklen_t sin_siz;
	struct sockaddr_in addr;
	struct sockaddr_in clt;
	struct timeval waitval;
	int yes = 1;
	int n;
	int maxfd;
	fd_set fds, readfds;
	struct sockaddr_in addr1, addr2;

	printf("Now Setting...\n");
	waitval.tv_sec		 = 2;
	waitval.tv_usec		 = 500;
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
	FD_SET(listen_sock, &readfds);

	printf("Ready to Start\n");
	while(1) {
		memcpy(&fds, &readfds, sizeof(fd_set));
		n = select(maxfd + 1, &fds, NULL, NULL, &waitval);
		if(FD_ISSET(listen_sock, &fds)) {
			sin_siz		= sizeof(clt);
			accept_sock = accept(listen_sock, (struct sockaddr*)&clt, &sin_siz);
			printf("accepted connection from %s, port=%d\n", inet_ntoa(clt.sin_addr), ntohs(clt.sin_port));
			close(accept_sock);
		}
		/* for(int i = 0; i < maxfd + 1; i++) {
			if(FD_ISSET(c_sock[i], &fds)) {
			}
		} */
	}
}