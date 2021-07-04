#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSIZE 1024
#define NUMBER_OF_SOCK 50
#define NUMBER_OF_LOG 10

/*
void reset_structure_client_member(struct client_member* p){
    memset((char *)&client_member, 0, sizeof(client_member));
}
*/


bool search_restricted_words(char str[], char found_restricted_word[], char* wordlist[BUFSIZE], int restricted_word_num){
    for(int i = 0; i < restricted_word_num; i++){
        if(strstr(str, wordlist[i]) != NULL){
            //printf("[ ! ] An restricted_ word was found from your string : \"%s\"\n", wordlist[i]);
            strcpy(found_restricted_word, wordlist[i]);
            return true;
        }
    }
    return false;
}

//簡易ハッシュ関数
int my_lcg_encryptor(char str[]){
    int len = strlen(str);
    int i;
    unsigned int result = 1;

    for(i = 0; i < len; i++){
        result *= (int)str[i];
    }

    // LCG by Park & Millar
    for(i = 0; i < 100; i++){
        result *= 48271;
    }

    return result;
}



// ./server.exe [port] [data to send]
int main(int argc, char *argv[])
{
    int i, j;
    fd_set fds;

    //コマンドライン引数の個数の検査と説明
    if (argc != 2)
    {
        printf("Usage : %s [port]\n", argv[0]);
        exit(1);
    }
    printf("executed command: ");
    for(i = 0; i < argc; i++){
	printf("%s ", argv[i]);
    }
    printf("\n");

    int s[NUMBER_OF_SOCK] = { -1 };
    for (i = 0; i < NUMBER_OF_SOCK; i++)
    {
        s[i] = -1;
    }

    int port = atoi(argv[1]);

    //ソケットの生成と失敗時の対応
    s[0] = socket(AF_INET, SOCK_STREAM, 0);
    if (s[0] < 0)
    {
        fprintf(stderr, "[ ! ] Failed to create socket.\n");
        exit(1);
    }

    //sockaddr_in構造体型のmy_addrを作成
    struct sockaddr_in my_addr;
    //my_addrのゼロ初期化
    memset((char *)&my_addr, 0, sizeof(my_addr));

    // format of sockaddr_in...
    // 0                               31
    // +----------------+---------------+
    // |   sin_family   |   sin_port    |
    // +----------------+---------------+
    // |            sin_addr            |
    // +--------------------------------+
    // |             UNUSED             |
    // +--------------------------------+
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(port);

    // sockaddr_in型からsockaddr型に再変換する
    if (bind(s[0], (struct sockaddr *)&my_addr, sizeof(my_addr)))
    {
        fprintf(stderr, "[ ! ] Failed to bind socket.\n");
        exit(1);
    }
    printf("[ + ] Binded socket successfully.\n");

    if (listen(s[0], 10) < 0)
    {
        fprintf(stderr, "[ ! ] Failed to listen.\n");
        exit(1);
    }
    printf("[ + ] listen started successfully.\n");

    int new_s;
    socklen_t addr_len;
    //sockaddr_in構造体型のclient_addrを作成
    struct sockaddr_in client_addr;
    //my_addrのゼロ初期化
    
    //コマンド関連設定
    char command_name[BUFSIZE];
    char command_parameter[BUFSIZE];
    int command;
    char* command_name_end_index;
    enum CmdMode{
        CHAT,
        LOGIN
    };
    enum AuthLevel{
        USER,
        ADMIN
    };
    
    struct message_log{
        time_t time;
        char message[BUFSIZE];
    };

    struct clients{
        char nickname[32];
        char ip[16];
        enum CmdMode cmd_mode;
        enum AuthLevel auth_level;
        int number_of_speak;
        struct message_log log[NUMBER_OF_LOG];
    };
    

    //システムのための変数
    char rec_buf[BUFSIZE];
    char send_buf[BUFSIZE*2];
    int n;
    int ret;

    //諸機能のための変数
    char client_ips[NUMBER_OF_SOCK][16];
    struct clients clients[NUMBER_OF_SOCK];
    struct message_log messages[NUMBER_OF_SOCK][NUMBER_OF_LOG];
    char time_information[BUFSIZE];
    char* restricted_wordlist[BUFSIZE] = {"Unko", "unko", "veve"};
    char found_restricted_word[BUFSIZE];

    
    

    

    time_t timer = time(NULL);
    time_t prev_time;
    struct tm* timeptr;

    while(1){
        FD_ZERO(&fds);

        printf("---------- socket status (array of accepted socket id) ------------\n");
        for (i = 0; i < NUMBER_OF_SOCK; i++){
            printf("%d ",s[i]);
        }
        printf("\n---------- socket status end ------------\n");

        for(i = 0; i < NUMBER_OF_SOCK; i++){
            if (s[i] != -1) { FD_SET(s[i], &fds); }
        }
        if ((n = select(FD_SETSIZE, &fds, NULL, NULL, NULL)) == -1)
        {
            perror("select");
            fprintf(stderr, "select() error.\n");
            exit(1);
        }

        //新たな接続要求の処理
        if(s[0] != -1 && FD_ISSET(s[0], &fds)){

            for(i = 1; i < NUMBER_OF_SOCK; i++){
                if(s[i] == -1){ break; }
            }

            memset((char *)&client_addr, 0, sizeof(client_addr));

            addr_len = sizeof(client_addr);
            if ((s[i] = accept(s[0], (struct sockaddr *)&client_addr, &addr_len)) < 0)
            {
                fprintf(stderr, "[ ! ] Failed to accept socket.\n");
                exit(1);
            }

            printf("[ + ] Accepted a socket successfully.\n");

            //ニックネームの受信と受信失敗時の対応
            if ((n = recv(s[i], clients[i].nickname, BUFSIZE, 0)) < 0)
            {
                fprintf(stderr, "[ ! ] Failed to receive nickname data\n");
                continue;
            }
            clients[i].nickname[n] = '\0';

            //クライアントのデータの初期化
            strcpy(clients[i].ip, inet_ntoa(client_addr.sin_addr)); 
            clients[i].number_of_speak = 0;
            clients[i].cmd_mode = CHAT;
            clients[i].auth_level = USER;
            
	    printf("[ + ] Accepted an user! username: %s, IP: %s, fd: %d\n",
	           clients[i].nickname,
		   clients[i].ip,
		   s[i]);

            write(s[i], "+----------------------------------------------------------+\n", BUFSIZE);
            write(s[i], "|                        CHAT STARTED !!                   |\n", BUFSIZE);
            write(s[i], "+----------------------------------------------------------+\n", BUFSIZE);
        }
        
        //受信の有無の確認，および受信後処理
        for(i = 1; i < NUMBER_OF_SOCK; i++){
            if(s[i] != -1 && FD_ISSET(s[i], &fds)){
                ret = read(s[i], rec_buf, BUFSIZE);
                
                //クライアントが接続を切った場合の処理
                if(ret == 0){
                    write(s[i], "Good bye.\n", BUFSIZE);
                    FD_CLR(s[i], &fds);
                    close(s[i]);
                    s[i] = -1;
                    continue;
                }

                //NGワードの処理
                if(search_restricted_words(rec_buf, found_restricted_word, restricted_wordlist, 3)){
                    write(s[i], "[ ! ] An restricted word was found from your string: \"", BUFSIZE);
                    write(s[i], found_restricted_word, BUFSIZE);
                    write(s[i], "\"\n", BUFSIZE);
                    continue;
                }

                //コマンド入力状態時の処理
                if(clients[i].cmd_mode != CHAT){
                    switch(clients[i].cmd_mode){
                        case LOGIN:
                            
                            break;

                    }
                }

                //コマンド入力感知の処理
                if(rec_buf[0] == '#'){
                    strcpy(command_name, rec_buf + 1);
                    printf("%s\n", command_name);

                    if(strcmp(command_name, "login")){
                        clients[i].cmd_mode = LOGIN;
                        write(s[i], "[ i ] Input Admin password : ", BUFSIZE);
                    }

                    
                }

                // ----- 通常発言 -----

                //
                if(clients[i].number_of_speak > NUMBER_OF_LOG){
                    if(time(NULL) - clients[ i ].log[ clients[i].number_of_speak % NUMBER_OF_LOG ].time < 30){
                        write(s[i], "[ ! ] You sended too many posts in a short-time.\n", BUFSIZE);
                        continue;
                    }
                }

                clients[i].log[ clients[i].number_of_speak % NUMBER_OF_LOG ].time = time(NULL);

                //時刻情報の取得
                timer = time(NULL);
                timeptr = localtime(&timer);
                strftime(time_information, BUFSIZE, "%Y/%m/%d %T", timeptr);
		
		//サーバログにメッセージを表示
		printf("[+] %s(%s) send a message: %s",
	               clients[i].nickname,
		       clients[i].ip,
		       rec_buf);
                
		//発言を周知する
                for (j = 1; j < NUMBER_OF_SOCK; j++)
                {
		    if(s[i] != -1 && i != j){ //メッセージ送信元以外のクライアントにメッセージを送信 
                        
                        strcpy(send_buf, "\n------------------------------------------------------------\n"); //'-'*60
                        strcat(send_buf, clients[i].nickname);
                        strcat(send_buf, "(");
                        strcat(send_buf, clients[i].ip);
                        strcat(send_buf, ") - ");
                        strcat(send_buf, time_information);
                        strcat(send_buf, "\n");
                        strcat(send_buf, rec_buf);
                        strcat(send_buf, "------------------------------------------------------------\n"); //'-'*60
                        write(s[j], send_buf, BUFSIZE);
                    }
                }

                clients[i].number_of_speak += 1;
            }
        }
    }

    return 0;
}
