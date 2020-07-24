#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>

const int MAX_EVENTS = 10;

void error(char* msg);
int read_line(int socket, char* buf, int len);
int substr(char* t, const char* s, int pos, int len);

int main(int argc, char* argv[]) {
	struct sockaddr_in name;
	struct sockaddr_storage client_addr;
	struct epoll_event ev;
	struct epoll_event events[MAX_EVENTS];
	unsigned int address_size = sizeof(client_addr);
	int epfd, listener_d;
	int num_fd = 0;
	int* fd_list;			 //接続しているユーザのソケット番号
	char** user_list;		 //接続しているユーザの名前

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

	printf("Ready ...\n");
	while(1) {
		int fd_count = epoll_wait(epfd, events, MAX_EVENTS, -1);
		char buf[255];

		// 準備ができたディスクリプタを順番に処理
		int i;
		for(i = 0; i < fd_count; i++) {
			if(events[i].data.fd == listener_d) {
				int connect_d = accept(listener_d, (struct sockaddr*)&client_addr, &address_size);
				if(connect_d == -1) {
					error("accept err");
				}

				printf("%d: Client Connected", connect_d);
				char* msg = "Welcome!\r\n";
				write(connect_d, msg, strlen(msg));

				memset(&ev, 0, sizeof ev);
				ev.events  = EPOLLIN;
				ev.data.fd = connect_d;
				if((epoll_ctl(epfd, EPOLL_CTL_ADD, connect_d, &ev)) < 0) {
					error("epoll_ctl error");
				}

				/* 新しくチャットに入ったクライアントの番号を配列として保存する */
				if(num_fd == 0) {
					fd_list		 = (int*)malloc(sizeof(int));
					user_list	 = (char**)malloc(sizeof(char));
					user_list[0] = (char*)malloc(sizeof(char) * 255);
					fd_list[0]	 = connect_d;
				} else {
					/* データのコピー */
					int* fd_buf		= (int*)malloc(sizeof(int) * num_fd);
					char** user_buf = (char**)malloc(sizeof(char) * num_fd);
					for(int user_num = 0; user_num <= num_fd; user_num++) {
						user_buf[user_num] = (char*)malloc(sizeof(char) * 255);
						memcpy(user_buf[user_num], user_list[user_num], sizeof(user_list[user_num]));
						free(user_buf[user_num]);
					}
					memcpy(fd_buf, fd_list, sizeof(fd_buf));
					free(fd_list);
					free(user_list);

					/* fd配列の要素を1つ増やし，fd_bufからコピーする */
					fd_list = (int*)malloc(sizeof(int) * (num_fd + 1));
					memcpy(fd_list, fd_buf, sizeof(fd_buf));
					free(fd_buf);

					/* user配列の要素を1つ増やし，user_bufからコピーする */
					user_list = (char**)malloc(sizeof(char) * (num_fd + 1));
					for(int user_num = 0; user_num <= num_fd; user_num++) {
						user_list[user_num] = (char*)malloc(sizeof(char) * 255);
						memcpy(user_list[user_num], user_buf[user_num], sizeof(user_buf[user_num]));
						free(user_buf[user_num]);
					}
					free(user_buf);

					/* 新しいユーザを登録 */
					fd_list[num_fd] = connect_d;
					memset(user_buf, '\0', sizeof(user_buf));
					strcpy(user_buf[num_fd], "Anonymous");
				}
				num_fd++;
				printf("\n");
			} else {
				int connect_d = events[i].data.fd;
				read(connect_d, buf, sizeof(buf));

				if(strncmp(buf, ":q", 2) == 0) {		//終了コマンド
					/* クライアントを切断する */
					printf("%d: Client Closed\n", connect_d);
					close(connect_d);
					epoll_ctl(epfd, EPOLL_CTL_DEL, connect_d, &ev);

					/* 切断するクライアント以外をコピー */
					int* fd_buf		= (int*)malloc(sizeof(int) * num_fd - 1);
					char** user_buf = (char**)malloc(sizeof(char) * num_fd - 1);
					for(int user_num = 0; user_num <= num_fd; user_num++) {
						user_buf[user_num] = (char*)malloc(sizeof(char) * 255);
					}
					int fd_buf_num = 0;
					for(int k = 0; k < num_fd; k++) {
						if(fd_list[k] != connect_d) {
							fd_buf[fd_buf_num]	 = fd_list[k];
							user_buf[fd_buf_num] = user_list[k];
							fd_buf_num++;
							free(user_list[k]);
						}
					}
					free(user_list);
					free(fd_list);

					/* クライアントのfdリストを更新 */
					fd_list = (int*)malloc(sizeof(int) * (fd_buf_num));
					memcpy(fd_list, fd_buf, sizeof(fd_buf));
					free(fd_buf);
					user_list = (char**)malloc(sizeof(char) * (fd_buf_num));
					for(int user_num = 0; user_num <= fd_buf_num; user_num++) {
						user_buf[user_num] = (char*)malloc(sizeof(char) * 255);
						memcpy(user_list[user_num], user_buf[user_num], sizeof(user_buf[user_num]));
						free(user_buf[user_num]);
					}
					free(user_buf);

					num_fd--;
				} else if(strncmp(buf, ":u", 2) == 0) {
					char user_name[255];
					strncpy(user_name, buf + 3, 252);
					printf("%d: Client User Name: %s\n", connect_d, user_name);
					memset(buf, '\0', sizeof(buf));

					for(int user_num = 0; user_num <= sizeof(fd_list); user_num++) {
						if(fd_list[user_num] = connect_d) {
							printf("aaaaa\n");
							memset(user_list[user_num], '\0', sizeof(user_list[user_num]));
							printf("bbbbb\n");
							strcpy(user_list[user_num], user_name);
							printf("ccccc\n");
						}
					}
				} else {
					char writeData[255];
					sprintf(writeData, "%d: %s", connect_d, buf);
					printf("%s", writeData);
					for(int k = 0; k < num_fd; k++) {
						if(fd_list[k] != connect_d) {
							write(fd_list[k], writeData, strlen(writeData));
						}
					}
					memset(buf, '\0', sizeof(buf));
				}
			}
		}
	}
	return 0;
}

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