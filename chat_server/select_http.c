#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void* send_recv_thread(void* arg);

int main() {
	int epfd, nfd, fd;
	struct epoll_event event;
	struct epoll_event events;
	struct sockaddr_in addr;
	struct sockaddr_in clt;
	char s[2048];
	int listen_sock, accept_sock;
	socklen_t sin_siz;
	int yes = 1;
	int cnt;
	char buf[2048];

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

	epfd = epoll_create(1);
	if(epfd < 0) {
		printf("epoll_create() failed¥n");
		return -1;
	}

	memset(&event, 0, sizeof(event));
	event.events  = EPOLLIN;
	event.data.fd = 0;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &event) < 0) {
		printf("epoll_ctl() failed¥n");
		return -1;
	}

	printf("Ready to Start\n");
	while(1) {
		nfd = epoll_wait(epfd, &events, 1, -1);
		printf("%d\n", nfd);
		if(nfd < 0) {
			printf("epoll_wait() failed¥n");
		} else if(nfd == 0) {
			printf("epoll_wait() timeout¥n");
		} else if(nfd) {
			cnt = recv(listen_sock, buf, sizeof(buf), 0);
			read(fd, s, sizeof(s));
			printf("%s", s);
			memset(&s, 0, sizeof(s));
		}
	}
}
