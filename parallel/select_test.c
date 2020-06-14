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
	int fd;				// 接続待ち用ディスクリプタ
	int fd2[10];		// 通信用ディスクリプタの配列
	struct servent* serv;
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);
	struct sockaddr_in from_addr;
	char buf[2048];
	int cnt;
	int maxfd;				  // ディスクリプタの最大値
	fd_set rfds;			  // 接続待ち、受信待ちをするディスクリプタの集合
	struct timeval tv;		  // タイムアウト時間

	printf("Now Setting...\n");
	// 受信バッファを初期化する
	memset(buf, 0, sizeof(buf));
	// 通信用ディスクリプタの配列を初期化する
	for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
		fd2[i] = -1;
	}
	// ソケットを作成する
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stdout, "socket error : fd = %d\n", fd);
		return -1;
	}

	addr.sin_family		 = AF_INET;
	addr.sin_port		 = ntohs(22629);
	addr.sin_addr.s_addr = INADDR_ANY;
	// バインドする
	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		fprintf(stdout, "bind error\n");
		return -1;
	}
	// 接続待ち状態とする。待ちうけるコネクト要求は１個
	if(listen(fd, 1) < 0) {
		fprintf(stdout, "listen error\n");
		return -1;
	}

	printf("Ready to Start\n");

	while(1) {
		// 接続待ちのディスクリプタをディスクリプタ集合に設定する
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		maxfd = fd;
		// 受信待ちのディスクリプタをディスクリプタ集合に設定する
		for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
			if(fd2[i] != -1) {
				FD_SET(fd2[i], &rfds);
				if(fd2[i] > maxfd)
					maxfd = fd2[i];
			}
		}
		// タイムアウト時間を10sec+500000μsec に指定する。
		tv.tv_sec  = 10;
		tv.tv_usec = 500000;

		// 接続＆受信を待ち受ける
		cnt = select(maxfd + 1, &rfds, NULL, NULL, &tv);
		if(cnt < 0) {
			// シグナル受信によるselect終了の場合、再度待ち受けに戻る
			if(errno == EINTR)
				continue;
			// その他のエラーの場合、終了する。
			goto end;
		} else if(cnt == 0) {
			// タイムアウトした場合、再度待ち受けに戻る
			continue;
		} else {
			// 接続待ちディスクリプタに接続があったかを調べる
			if(FD_ISSET(fd, &rfds)) {
				// 接続されたならクライアントからの接続を確立する
				for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
					if(fd2[i] == -1) {
						if((fd2[i] = accept(fd, (struct sockaddr*)&from_addr, &len)) < 0) {
							goto end;
						}
						fprintf(stdout, "socket:%d  connected. \n", fd2[i]);
						break;
					}
				}
			}
			for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
				// 受信待ちディスクリプタにデータがあるかを調べる
				if(FD_ISSET(fd2[i], &rfds)) {
					// データがあるならパケット受信する
					cnt = recv(fd2[i], buf, sizeof(buf), 0);
					if(cnt > 0) {
						// パケット受信成功の場合
						fprintf(stdout, "recv:\"%s\"\n", buf);
					} else if(cnt == 0) {
						// 切断された場合、クローズする
						fprintf(stdout, "socket:%d  disconnected. \n", fd2[i]);
						close(fd2[i]);
						fd2[i] = -1;
					} else {
						goto end;
					}
				}
			}
		}
	}
end:
	// パケット送受信用ソケットクローズ
	for(int i = 0; i < sizeof(fd2) / sizeof(fd2[0]); i++) {
		close(fd2[i]);
	}
	// 接続要求待ち受け用ソケットクローズ
	close(fd);
	return 0;
}