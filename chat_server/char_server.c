#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>

const int MAX_EVENTS = 10;

void error(char* msg) {
	fprintf(stderr, "%s:%s\n", msg, strerror(errno));
	exit(1);
}

int read_line(int socket, char* buf, int len) {
	char* s	 = buf;
	int slen = len;
	int c	 = read(socket, s, slen);
	while((c > 0) && (s[c - 1] != '\n')) {
		s += c;
		slen = -c;
		c	 = read(socket, s, slen);
	}
	if(c < 0) {
		return c;
	}
	return len - slen;
}

int main(int argc, char* argv[]) {
	struct sockaddr_in name;
	struct sockaddr_storage client_addr;
	struct epoll_event ev;
	struct epoll_event events[MAX_EVENTS];
	unsigned int address_size = sizeof(client_addr);
	char buf[255];
	int epfd;
	int listener_d;

	listener_d = socket(PF_INET, SOCK_STREAM, 0);
	if(listener_d == -1) {
		error("socket err");
	}
	name.sin_family		 = AF_INET;
	name.sin_port		 = htons(22629);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(listener_d, (struct sockaddr*)&name, sizeof(name)) == -1) {
		error("bind err");
	}

	if(listen(listener_d, 1) == -1) {
		error("listen err");
	}

	puts("wait...");

	// epollファイルディスクリプタをオープン
	if((epfd = epoll_create(100)) < 0) {
		error("epoll_create err");
	}

	// listener_dソケットをepollの監視対象とする
	memset(&ev, 0, sizeof ev);
	ev.events  = EPOLLIN;
	ev.data.fd = listener_d;
	if((epoll_ctl(epfd, EPOLL_CTL_ADD, listener_d, &ev)) < 0) {
		error("epoll_ctl error");
	}

	while(1) {
		int fd_count = epoll_wait(epfd, events, MAX_EVENTS, -1);

		// 準備ができたディスクリプタを順番に処理
		int i;
		for(i = 0; i < fd_count; i++) {
			if(events[i].data.fd == listener_d) {
				int connect_d = accept(listener_d, (struct sockaddr*)&client_addr, &address_size);
				if(connect_d == -1) {
					error("accept err");
				}

				printf("Client Connected\n");
				char* msg = "Welcome!\r\n";
				write(connect_d, msg, strlen(msg));

				// connect_dソケットを監視対象とする
				memset(&ev, 0, sizeof ev);
				ev.events  = EPOLLIN;
				ev.data.fd = connect_d;
				if((epoll_ctl(epfd, EPOLL_CTL_ADD, connect_d, &ev)) < 0) {
					error("epoll_ctl error");
				}
			} else {
				int connect_d = events[i].data.fd;
				read_line(connect_d, buf, sizeof(buf));
				printf("%d : %s", connect_d, buf);

				if(strncmp(buf, ":q", 2) == 0) {		//終了コマンド
					printf("close\n");
					close(connect_d);
					epoll_ctl(epfd, EPOLL_CTL_DEL, connect_d, &ev);
				} else {
					for(int j = 0; j < fd_count; j++) {
						if(j != i) {
							int connect_j = events[j].data.fd;
							if(connect_j == -1) {
								continue;
							}
							printf("%d\n", connect_j);
							write(connect_j, buf, strlen(buf));
						}
					}
				}
			}
		}
	}
	return 0;
}