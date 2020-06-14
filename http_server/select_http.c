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
	int listen_sock;								   // 接続待ち用ディスクリプタ
	int fd2[10];									   // 通信用ディスクリプタの配列
	struct sockaddr_in addr;						   // サーバー
	socklen_t len = sizeof(struct sockaddr_in);		   // 長さ
	struct sockaddr_in from_addr;					   // クライアント
	char buf[2048];									   // バッファ
	int cnt;										   // cnt
	int maxfd;										   // ディスクリプタの最大値
	fd_set rfds;									   // 接続待ち、受信待ちをするディスクリプタの集合
	struct timeval tv;								   // タイムアウト時間
	int yes = 1;									   //
	char inbuf[2048];
	char obuf[2048];

	printf("Now Setting...\n");
	memset(buf, 0, sizeof(buf));								   // 受信バッファを初期化する
	for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {		   // 通信用ディスクリプタの配列を初期化する
		fd2[i] = -1;
	}

	listen_sock			 = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family		 = AF_INET;
	addr.sin_port		 = ntohs(22629);
	addr.sin_addr.s_addr = INADDR_ANY;
	if(listen_sock < 0) {
		perror("socket");
		return -1;
	}

	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));		  //TIME_WAIT状態でも再起動可能に設定
	if(bind(listen_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return -1;
	}
	if(listen(listen_sock, 1) < 0) {		// 接続待ち状態とする。待ちうけるコネクト要求は１個
		perror("listen");
		return -1;
	}

	/* HTTPメッセージ作成 */
	memset(obuf, 0, sizeof(obuf));
	snprintf(obuf, sizeof(obuf), "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<font color=red><h1>HELLO</h1></font>\r\n");

	printf("Ready to Start\n");
	while(1) {
		// 接続待ちのディスクリプタをディスクリプタ集合に設定する
		FD_ZERO(&rfds);
		FD_SET(listen_sock, &rfds);
		maxfd = listen_sock;
		for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {		   // 受信待ちのディスクリプタをディスクリプタ集合に設定する
			if(fd2[i] != -1) {
				FD_SET(fd2[i], &rfds);
				if(fd2[i] > maxfd)
					maxfd = fd2[i];
			}
		}

		// タイムアウト時間を10sec+500000μsec に指定する。
		tv.tv_sec  = 10;
		tv.tv_usec = 500000;

		cnt = select(maxfd + 1, &rfds, NULL, NULL, &tv);		// 接続＆受信を待ち受ける
		if(cnt < 0) {											// シグナル受信によるselect終了の場合、再度待ち受けに戻る
			if(errno == EINTR)									// その他のエラーの場合、終了する。
				continue;
			// パケット送受信用ソケットクローズ
			for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
				close(fd2[i]);
			}
			close(listen_sock);		   // 接続要求待ち受け用ソケットクローズ
			return 0;
		} else if(cnt == 0) {		 // タイムアウトした場合、再度待ち受けに戻る
			continue;
		} else {									  // 接続待ちディスクリプタに接続があったかを調べる
			if(FD_ISSET(listen_sock, &rfds)) {		  // 接続されたならクライアントからの接続を確立する
				for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
					if(fd2[i] == -1) {
						if((fd2[i] = accept(listen_sock, (struct sockaddr*)&from_addr, &len)) < 0) {
							// パケット送受信用ソケットクローズ
							for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
								close(fd2[i]);
							}
							close(listen_sock);		   // 接続要求待ち受け用ソケットクローズ
							return 0;
						}
						fprintf(stdout, "socket:%d  connected. \n", fd2[i]);
						break;
					}
				}
			}
			for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
				// 受信待ちディスクリプタにデータがあるかを調べる
				if(FD_ISSET(fd2[i], &rfds)) {		 // データがあるならパケット受信する
					cnt = recv(fd2[i], buf, sizeof(buf), 0);
					if(cnt > 0) {
						// パケット受信成功の場合
						fprintf(stdout, "recv:\"%s\"\n", buf);
						printf("%s", inbuf);
						send(fd2[i], obuf, (int)strlen(obuf), 0);
					} else if(cnt == 0) {		 // 切断された場合、クローズする
						fprintf(stdout, "socket:%d  disconnected. \n", fd2[i]);
						close(fd2[i]);
						fd2[i] = -1;
					} else {
						// パケット送受信用ソケットクローズ
						for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
							close(fd2[i]);
						}
						close(listen_sock);		   // 接続要求待ち受け用ソケットクローズ
						return 0;
					}
				}
			}
		}
	}
}