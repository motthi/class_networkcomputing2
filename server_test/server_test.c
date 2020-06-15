#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int main() {
	struct sockaddr_in addr;
	struct sockaddr_in client;
	int len;
	int sock;
	int cli;
	int yes = 1;

	sock				 = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family		 = AF_INET;
	addr.sin_port		 = htons(22629);
	addr.sin_addr.s_addr = INADDR_ANY;

	printf("Now Setting...\n");
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));		   //TIME_WAIT状態でも再起動可能に設定
	if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {					   //通信元の情報を与える
		perror("bind");
		return 0;
	}
	if(listen(sock, 5) == -1) {		   //用意したsocketを待ち受け状態にする（CLOSED->LISTEN)
		perror("listen");
		return 0;
	}

	printf("Ready to Start\n");
	while(1) {
		len = sizeof(client);
		cli = accept(sock, (struct sockaddr*)&client, &len);		//クライアントから受信
		if(cli == -1) {
			perror("accept");
		}
		printf("accepted connection from %s, port=%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		write(cli, "Hello\n", 7);
		close(cli);
	}
}
