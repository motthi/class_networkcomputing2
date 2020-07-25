#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>

const int MAX_EVENTS = 10;

int main(int argc, char* argv[]) {
	struct sockaddr_in name;
	struct sockaddr_storage client_addr;
	struct epoll_event ev;
	struct epoll_event events[MAX_EVENTS];
	unsigned int address_size = sizeof(client_addr);
	int epfd, listener_d;
	int num_fd = 0;
	int yes	   = 1;
	int* fd_list;			 //接続しているユーザのソケット番号
	char** user_list;		 //接続しているユーザの名前

	/* サーバーの設定 */
	listener_d = socket(PF_INET, SOCK_STREAM, 0);
	if(listener_d == -1) {
		perror("socket");
		return -1;
	}
	name.sin_family		 = AF_INET;
	name.sin_port		 = htons(22629);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	setsockopt(listener_d, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
	if(bind(listener_d, (struct sockaddr*)&name, sizeof(name)) == -1) {
		perror("bind");
		return -1;
	}
	if(listen(listener_d, 1) == -1) {
		perror("listen");
		return -1;
	}

	// epollファイルディスクリプタをオープン
	if((epfd = epoll_create(100)) < 0) {
		perror("epoll_create");
		return -1;
	}

	// listener_dソケットをepollの監視対象とする
	memset(&ev, 0, sizeof ev);
	ev.events  = EPOLLIN;
	ev.data.fd = listener_d;
	if((epoll_ctl(epfd, EPOLL_CTL_ADD, listener_d, &ev)) < 0) {
		perror("epoll_ctl");
		return -1;
	}

	printf("Ready ...\n");
	while(1) {
		int fd_count = epoll_wait(epfd, events, MAX_EVENTS, -1);
		char buf[255];

		/* 準備ができたディスクリプタを順番に処理 */
		int i;
		for(i = 0; i < fd_count; i++) {
			if(events[i].data.fd == listener_d) {		 //新しいクライアントがサーバーに接続した場合
				int connect_d = accept(listener_d, (struct sockaddr*)&client_addr, &address_size);
				if(connect_d == -1) {
					perror("accept");
				}

				/* Welcomeメッセージをクライアントに送信 */
				printf("%d\t: Client Connected", connect_d);
				char* msg = "Welcome!\r\n";
				write(connect_d, msg, strlen(msg));

				memset(&ev, 0, sizeof ev);
				ev.events  = EPOLLIN;
				ev.data.fd = connect_d;
				if((epoll_ctl(epfd, EPOLL_CTL_ADD, connect_d, &ev)) < 0) {
					perror("epoll_ctl");
				}

				/* 新しくチャットに入ったクライアントの番号を配列として保存する */
				if(num_fd == 0) {		 //チャットに初めてクライアントがアクセスした場合
					fd_list		 = (int*)malloc(sizeof(int));
					user_list	 = (char**)malloc(sizeof(char));
					user_list[0] = (char*)malloc(sizeof(char) * 255);
					fd_list[0]	 = connect_d;
				} else {		//すでにクライアントがいる場合
					/* データのコピー */
					int* fd_buf		= (int*)malloc(sizeof(int) * num_fd);
					char** user_buf = (char**)malloc(sizeof(char) * num_fd);
					for(int user_num = 0; user_num <= num_fd - 1; user_num++) {
						user_buf[user_num] = (char*)malloc(sizeof(char) * 255);
						memcpy(user_buf[user_num], user_list[user_num], sizeof(user_list[user_num]));
						free(user_list[user_num]);
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
					for(int user_num = 0; user_num <= num_fd - 1; user_num++) {
						user_list[user_num] = (char*)malloc(sizeof(char) * 255);
						memcpy(user_list[user_num], user_buf[user_num], sizeof(user_buf[user_num]));
						free(user_buf[user_num]);
					}
					free(user_buf);

					/* 新しいユーザを登録 */
					fd_list[num_fd]	  = connect_d;
					user_list[num_fd] = (char*)malloc(sizeof(char) * 255);
					memset(user_list[num_fd], '\0', sizeof(user_list));
					strcpy(user_list[num_fd], "Anonymous");
				}
				num_fd++;
				printf("\n");
			} else {
				int connect_d = events[i].data.fd;
				read(connect_d, buf, sizeof(buf));

				if(strncmp(buf, ":q", 2) == 0) {		//終了コマンド
					/* クライアントを切断する */
					printf("%d\t: Client Closed\n", connect_d);
					close(connect_d);
					epoll_ctl(epfd, EPOLL_CTL_DEL, connect_d, &ev);

					/* 切断するクライアント以外をコピー */
					int* fd_buf		= (int*)malloc(sizeof(int) * num_fd - 1);
					char** user_buf = (char**)malloc(sizeof(char) * num_fd - 1);
					for(int user_num = 0; user_num <= num_fd - 1; user_num++) {
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
						user_list[user_num] = (char*)malloc(sizeof(char) * 255);
						memcpy(user_list[user_num], user_buf[user_num], sizeof(user_buf[user_num]));
						free(user_buf[user_num]);
					}
					free(user_buf);

					num_fd--;
					memset(buf, '\0', sizeof(buf));
				} else if(strncmp(buf, ":u", 2) == 0) {		   //クライアントのユーザー名を登録
					char user_name[255];
					strncpy(user_name, buf + 3, 252);
					printf("%d\t: Client User Name: %s\n", connect_d, user_name);

					/* 該当するクライアントの配列要素を検索し，ユーザー名を更新 */
					for(int user_num = 0; user_num <= sizeof(fd_list); user_num++) {
						if(fd_list[user_num] == connect_d) {
							memset(user_list[user_num], '\0', sizeof(user_list[user_num]));
							strcpy(user_list[user_num], user_name);
						}
					}
					memset(buf, '\0', sizeof(buf));
				} else {
					char writeData[255];
					int user_num = 0;

					/* クライアントを確認 */
					for(user_num = 0; user_num <= num_fd; user_num++) {
						if(fd_list[user_num] == connect_d) {
							break;
						}
					}

					/* 時々改行だけが入る，最初が改行の場合は無視 */
					if((int)buf[0] != 0x0A) {
						sprintf(writeData, "%s\t: %s", user_list[user_num], buf);
						printf("%s", writeData);

						/* 他のクライアントに送信 */
						for(int k = 0; k < num_fd; k++) {
							if(fd_list[k] != connect_d) {
								write(fd_list[k], writeData, strlen(writeData));
							}
						}
					}
					memset(buf, '\0', sizeof(buf));
				}
			}
		}
	}
	return 0;
}