#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <strings.h>
#include <netdb.h>
int sockfd; 
int opfd;
char account[120];
char passwd[120];
char tmp[1024];
char pixels[10];
int my_turn = 0;
int playing = 0;
char player1 = 'O';
char player2 = 'X';
char position[10];
int first;

int win(char mark);

void init()
{
    memset(position, ' ', sizeof(position));
    int port = 6543;
    sockfd = socket(PF_INET, SOCK_STREAM, 0);//create socket
    struct sockaddr_in server;
    server.sin_family = PF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if(connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        printf("Connection refused\n");
        exit(0);
    }
}

void* recv_thread(void* p)
{
    while(1)
    {
        char buffer[102400];
        memset(buffer, '\0', sizeof(buffer));
        if(recv(sockfd, buffer, sizeof(buffer), 0) <= 0)
        {
            exit(0);
        }
        else if(strncmp(buffer, "[NOTICE]", 8)==0)
        {
            printf("%s", buffer);
            char *ptr = strstr(buffer, ":");
            char *qtr = strstr(buffer, ")");
            *qtr = '\0';
            opfd = atoi(ptr+1);
            sprintf(buffer, "[SYSTEM]Do you accept?(yes/no)");
            printf("%s\n", buffer);
        }
        else if(strncmp(buffer, "AGREE", 5)==0)//sent from opponent
        {
            char *ptr = strstr(buffer, " ");
            ptr = strstr(ptr+1, " ");
            char *qtr = strstr(buffer, "\n");
            *qtr = '\0';
            printf("%s agrees to play with you\n", ptr+1);
            printf("Game starts\n");
            memset(position, ' ', sizeof(position));
            player1 = 'X';
            player2 = 'O';
            playing = 1;
            printf("Wait for opponent's reply\n");
        }
        else if(buffer[0]=='*')
        {
            int num = atoi(&buffer[1]);
            position[num] = player2;
            if(win(player2))
            {
                printf("LOSE QQQ\n");
                playing = 0;
                first = 1;
            }
            else if(fair())
            {
                printf("Fair\n");
                first = 1;
            }
            else
            {
                my_turn = 1;
                printf("\n");
                print_map();
                printf("Please Enter *0 to *8\n");
            }
        }
        else
            printf("%s", buffer);
    }
}

void log_in()
{
    printf("Password : ");
    fgets(passwd, 1024, stdin);
    passwd[strlen(passwd)-1] = '\0';
    sprintf(tmp, "%s:%s#", account, passwd);
}

void create()
{
    FILE *fp;
    char tmp[1024];
    char acc[1024];
    char pass[1024];
    fp = fopen("passwd", "a+");
    printf("Please your new account name : ");
    fgets(acc, 1024, stdin);
    printf("Please enter your password : ");
    fgets(pass, 1024, stdin);
    acc[strlen(acc)-1] = '\0';
    pass[strlen(pass)-1] = '\0';
    sprintf(tmp, "%s:%s\n", acc, pass);
    fwrite(tmp, strlen(tmp), 1, fp);
    fclose(fp);
}

void print_map()
{
    for(int i=0;i<3;i++)
    {
        for(int j=0;j<3;j++)
        {
            if(j!=2)
                printf("%c | ", position[3*i+j]);
            else
                printf("%c ", position[3*i+j]);
        }
        if(i!=2)
            printf("\n----------\n");
        else
            printf("\n");
    }
}

int win(char mark)
{
    if(position[0]==mark && position[1]==mark && position[2]==mark)
        return 1;
    if(position[3]==mark && position[4]==mark && position[5]==mark)
        return 1;
    if(position[6]==mark && position[7]==mark && position[8]==mark)
        return 1;
    if(position[0]==mark && position[3]==mark && position[6]==mark)
        return 1;
    if(position[1]==mark && position[4]==mark && position[7]==mark)
        return 1;
    if(position[2]==mark && position[5]==mark && position[8]==mark)
        return 1;
    if(position[0]==mark && position[4]==mark && position[8]==mark)
        return 1;
    if(position[2]==mark && position[4]==mark && position[6]==mark)
        return 1;
    return 0;
}

int fair()
{
    for(int i=0;i<9;i++)
    {
        if(position[i]==' ')
            return 0;
    }
    return 1;
}

void start()
{
    char *buffer;
    buffer = malloc(102400);
    int f = 1;
    while(1)
    {
        if(f==1)
            f = 0;
        else
            printf("\n");
        
        printf("Please Enter you account name or using NEW to register a new account\n");
        memset(buffer, '\0', sizeof(buffer));
        fgets(buffer, 1024, stdin);
        if(strcmp(buffer, "NEW\n")==0)
        {
            create();
        }
        else
        {
            memset(account, '\0', sizeof(account));
            buffer[strlen(buffer)-1] = '\0';
            strcpy(account, buffer);
            //printf("name = %s\n", account);
            log_in();
            memset(buffer, '\0', sizeof(buffer));
            //printf("now is %d\n", sockfd);
            recv(sockfd, buffer, sizeof(buffer), 0);
            if(strcmp(buffer,"fetch")==0)
            {
                char check[1024];
                //printf("---IF---\n");
                send(sockfd, tmp, strlen(tmp), 0);
                //break;
                memset(check, '\0', sizeof(check));
                recv(sockfd, check, sizeof(check), 0);
                //printf("check = %s\n", check);
                if(strcmp(check, "OK")==0)
                {
                    printf("login seccess!\n");
                    printf("\nWelcome %s!!!\n", account);
                    break;
                }    
                else
                {
                    printf("[WARN]your account doesn't exist or your password is wrong\n");
                    printf("[WARN]Please try again or Enter another account.\n");
                }
            }
        }
    }
    pthread_t id;
    pthread_create(&id, 0, recv_thread, 0);//handle data sent from server(in buffer)
    char *ptr;
    ptr = strstr(tmp, ":");
    *ptr = '\0';
    first = 1;
    while(1)
    {    
        char tmp[1024];
        memset(buffer, '\0', sizeof(buffer));
        fgets(buffer, 1024, stdin);//get input form keyboard
        if(playing==1 && my_turn==1 && first==1)
        {
            print_map();
            printf("Enter *0 to *8 to choose postion\n");
            first = 0;
        }
        if(strcmp(buffer, "exit\n")==0)
        {
            memset(tmp, '\0', sizeof(tmp));
            sprintf(tmp, "[SYSTEM]%s is leaving\n", account);
            send(sockfd, tmp, strlen(tmp), 0);
            break;
        }
        else if(strcmp(buffer, "ls\n")==0)
        {
            send(sockfd, "ls", 2, 0);
        }
        else if(buffer[0]=='#')
        {
            opfd = atoi(buffer+1);
            send(sockfd, buffer, strlen(buffer), 0);
        }
        else if(strcmp(buffer, "yes\n")==0)// AGREE/yes:only happen one of them
        {
            //printf("[SYSTEM]oppenet accepts\n");
            sprintf(buffer, "Play with %d\n", opfd);
            printf("[SYSTEM]%s", buffer);
            send(sockfd, buffer, strlen(buffer), 0);
            printf("[SYSTEM]Game starts\n");
            memset(position, ' ', sizeof(position));
            player1 = 'O';
            player2 = 'X';
            playing = 1;
            if(player1 == 'O')
                my_turn = 1;
            else
                my_turn = 0;
            if(my_turn==1)
                printf("Press Enter to continue\n");
            else
                printf("Wait for oppenent's reply\n");
        }
        else if(buffer[0]=='*')//choose position to check
        {
            if(playing==0)
            {
                printf("Isn't playing right now.\n");
            }
            else if(my_turn==0)
            {
                printf("Isn't your turn.\n");
            }
            else
            {
                int num = atoi(&buffer[1]);
                if(position[num]!=' ' || num<0 || num>8)
                    printf("Please Enter another number.\n");
                else
                {
                    position[num] = player1;
                    printf("\n");
                    print_map();
                    printf("\n");
                    if(win(player1))//my turn -> won't lose
                    {
                        printf("WINNING~~~\n");
                        playing = 0;
                        first = 1;
                    }
                    else if (fair())
                    {
                        printf("Fair QQ\n");
                        playing = 0;
                        first = 1;
                    }
                    else
                    {
                        printf("Waiting for your opponent's reply\n");
                    }
                    my_turn = 0;
                    sprintf(buffer, "*%d to %d", num, opfd);
                    send(sockfd, buffer, strlen(buffer), 0);   
                }
            }
        }
        else if(strcmp(buffer, "whoami\n")==0)
        {
            printf("NOW : %s\n", account);
        }
        else if(strcmp(buffer, "time\n")==0)
        {
            time_t timep;
            struct tm *p;
            time(&timep);
            p = gmtime(&timep);
            printf("[TIME]%d/%d/%d %d:%d:%d\n", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, 8+p->tm_hour, p->tm_min, p->tm_sec);
        }
        else if(strcmp(buffer, "\0")!=0)
        {
            //printf("size of buffer = %d\n", sizeof(buffer));
            send(sockfd, buffer, 10240, 0);
        }
    }
    close(sockfd);
}

int main()
{
    init();
    start();
    return 0;
}