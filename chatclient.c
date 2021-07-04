#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSIZE 1024

int main(int argc, char* argv[]){
    fd_set fds;
    int i;

    //コマンドライン引数の個数の検査と説明
    if (argc != 4)
    {
        printf("Usage : %s [IPv4_Addr] [port] [your username]\n", argv[0]);
        exit(1);
    }
    printf("executed command: ");
    for(i = 0; i < argc; i++){
	printf("%s ", argv[i]);
    }
    printf("\n");    

    // -------------------
    // TCPソケットの作成
    // -------------------
    int s;

    // ソケットの作成および生成の成否確認
    // sはソケット識別番号>0
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "[ ! ] Failed to create socket.\n");
        exit(1);
    }
    printf("[ + ] Socket created successfully : %d\n", s);

    // -----------------------------
    // コネクションの確立および成否確認
    // -----------------------------
    char *address = argv[1];
    struct sockaddr_in server_addr;
    memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(address);

    //port番号の設定および検査
    int port;
    port = atoi(argv[2]);
    if (port <= 0 && port > 65535)
    {
        printf("[ ! ] Irregal port number : \"%s\"\n", argv[2]);
        exit(1);
    }

    server_addr.sin_port = htons(port);

    int n;

    //コネクションの確立と失敗時の対応
    if (connect(s, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        fprintf(stderr, "[ ! ] Failed to establish connection to %s:%d.\n", argv[1], port);
        exit(1);
    }
    printf("[ + ] Connection established to %s:%d successfully.\n", argv[1], port);

    //ニックネーム送信と失敗時の対応
    char nickname[BUFSIZE];
    strcpy(nickname, argv[3]);
    int nickname_len = strlen(nickname);
    if((n = send(s, nickname, nickname_len, 0)) != nickname_len){
        fprintf(stderr, "[ ! ] Failed to send a message of your nickname: \"%s\"\n", nickname);
        exit(1);
    }
    printf("[ + ] Sended a message of your nickname successfully: \"%s\"\n", nickname);
    printf("[ + ] Now you can write and send arbitrary message here!\n");

    char readbuf[BUFSIZE];
    char stdinbuf[BUFSIZE];
    int ret;
    bool is_restricted_in;

    while(1){
        FD_ZERO(&fds);
        FD_SET(s, &fds);
        FD_SET(0, &fds);
	
	//監視対象のfdに受信があるまで待機
        if((n = select(FD_SETSIZE, &fds, NULL, NULL, NULL)) == -1){
            perror("select"); fprintf(stderr, "select() error\n"); exit(1);
        }
        
	//自分のソケット用fdに受信があった (他サーバからチャットメッセージを受け取った) 場合
        if(FD_ISSET(s, &fds)){
            bzero(readbuf, sizeof(readbuf));
            ret = read(s, readbuf, BUFSIZE);
            printf("%s", readbuf);
        }
        
	//標準入力があった場合, 読み込む
        if(FD_ISSET(0, &fds)){
            bzero(stdinbuf, sizeof(stdinbuf));
            fgets(stdinbuf, BUFSIZE, stdin);
            write(s, stdinbuf, BUFSIZE);
        }
    }

    return 0;
}
