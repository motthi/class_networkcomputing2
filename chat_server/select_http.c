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
	char s[2048];

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
	while(1) {
		nfd = epoll_wait(epfd, &events, 1, -1);
		if(nfd < 0) {
			printf("epoll_wait() failed¥n");
		} else if(nfd == 0) {
			printf("epoll_wait() timeout¥n");
		} else if(nfd) {
			read(fd, s, sizeof(s));
			printf("%s", s);
			memset(&s, 0, sizeof(s));
		}
	}
}