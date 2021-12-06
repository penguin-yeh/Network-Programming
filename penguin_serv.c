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
#define size 100
int sockfd_des;
int opfd;
int fds[120];
char account[120][1024];

void init()
{
    memset(fds, 0, sizeof(fds));
    //建立TCP socket(PF:Protocol Family,AF:Address Family,AF_INET = PF_INET)
    if((sockfd_des=socket(PF_INET, SOCK_STREAM, 0)) < 0)//create socket
    {
        printf("Create socket fail\n");
        exit(0);
    }
    srand(time(NULL));//for random_port
    struct sockaddr_in server;
    server.sin_family = PF_INET;
    //int random_port = rand() % 60000;
    int random_port = 6543;
    server.sin_port = htons(random_port);
    server.sin_addr.s_addr = INADDR_ANY;

    //Bind
    if(bind(sockfd_des, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("bind fails");
        exit(0);
    }
    puts("bind done\n");
    printf("bind to %d post\n", random_port);

    //listen
    if(listen(sockfd_des, 100) < 0)//most 100 clients at the same time
    {
        printf("listen fails\n");
        exit(0);
    }
}

int verify(char *str)//check account and passwd
{
    FILE *fp;
    char tmp[1024];
    fp = fopen("passwd", "rw");
    while(fscanf(fp, "%s", tmp)!=EOF) // wouldn't read \n
    {
        if(strcmp(tmp, str)==0)//find it!
            return 1;
    }
    fclose(fp);
    return 0;
}

void broadcast(char *str, int fd)
{
    char tmp[102400];
    //printf("str = %s\n", str);
    if(strcmp("\n", str)==0)
        return;
    if(str[strlen(str)-1]=='\n')
        str[strlen(str)-1] = '\0';
    if(strstr(str, "leaving"))
        sprintf(tmp, "%s\n", str);
    else
        sprintf(tmp, "[TALK]%s:%s\n", account[fd], str);
    printf("OOOOO\n");
    printf("tmp = %s\n", tmp);
    for(int i=0;i<size;i++)
    {
        if(fds[i]!=0)
        {
            printf("send to %d\n",fds[i]);
            send(fds[i], tmp, strlen(tmp), 0);
        }
    }
}

void* service_thread(void *p)//handling client
{
    int fd = *(int *)p;//transform type
    char buffer[102400];
    int tmp;

    printf("current pthread : %d\n", fd);
    //verify accounts to login
    while(1)
    {
        //printf("ewef");
        memset(buffer, '\0', sizeof(buffer));
        //printf("now is %d\n", fd);
        send(fd, "fetch", strlen("fetch"), 0);//fetch user's input
        recv(fd, buffer, 1024, 0);//receive from user
        printf("buffer : %s\n",buffer);
        char *ptr = strstr(buffer, "#");//client will mark # at the end
        *ptr = '\0';
        tmp = verify(buffer);//check if account and password are correct
        ptr = strstr(buffer, ":");
        *ptr = '\0';
        strcpy(account[fd], buffer);
        account[fd][strlen(buffer)] = '\0';
        *ptr = ':';//recover :
        if(tmp==1)//account exists
        {
            send(fd, "OK", 2, 0);
            printf("new accout : %s\n",account[fd]);
            break;
        }
        else
        {
            send(fd, "NOT OK", 6, 0);
            printf("account doesn't exist\n");
            //pthread_kill(fd, SIGALRM);
        }
    }
    //after login
    while(1)
    {
        char data[1024];
        char tmp;

        memset(buffer, '\0', sizeof(buffer));
        memset(data, '\0', 102400);
        if(recv(fd, buffer, 102400, 0) <= 0) // leave,ex:close(sockfd)
        {
            //printf("!!!!!!!!!!!!!!!!!!\n");
            for(int i=0;i<size;i++)
            {
                if(fds[i] == fd)
                {
                    fds[i] = 0;
                    break;
                }
            }
            printf("fd:%d quit\n",fd);
            pthread_exit(NULL);//parameter is return value 
        }
        if(strcmp(buffer, "ls")==0)
        {
            char str[1024] = "-------online-------\n";
            printf("ls\n");
            send(fd, str, strlen(str), 0);
            for(int i=0;i<size;i++)
            {
                if(fds[i]!=0)
                {
                    char tmp[100];
                    sprintf(tmp, "[SYSTEM]fd:%d user:%s\n", fds[i], account[fds[i]]);
                    send(fd, tmp, strlen(tmp), 0);
                }
            }
            strcpy(str, "--------------------\n");
            send(fd, str, strlen(str), 0);
        }
        else if(buffer[0] == '#')//choose opponent to challenge
        {
            opfd = atoi(&buffer[1]);
            printf("opfd = %d\n", opfd);
            sprintf(data, "[NOTICE]%s(fd:%d) wants to challenge you.\n", account[fd], fd);
            send(opfd, data, strlen(data), 0);//send to opponent
            sprintf(data, "[SYSYEM]Waiting for reply...\n");
            send(fd, data, strlen(data), 0);
        }
        else if(strncmp(buffer, "Play with", 9)==0)//opponent send this to server
        {
            //printf("@@@%s@@@", buffer);
            opfd = atoi(&buffer[10]);
            //printf("!!!! %d !!!!\n", atoi(&buffer[10]));
            printf("%s %d %d\n", account[fd], fd, opfd);
            sprintf(buffer, "AGREE from %s(fd:%d)\n", account[fd], fd);
            send(opfd, buffer, strlen(buffer), 0);//send to opponent's opponent
        }
        else if(buffer[0]=='*')//choose position to check
        {
            char *ptr = strstr(buffer, " ");
            *ptr = '\0';
            int num = atoi(&buffer[1]);
            *ptr = ' ';
            ptr = strstr(ptr+1, " ");
            opfd = atoi(ptr+1);
            sprintf(buffer, "*%d", num);
            send(opfd, buffer, strlen(buffer), 0);//notify opponent
        }
        else
        {
            printf("before BC\n");
            printf("buffer = %s\n", buffer);
            broadcast(buffer, fd);
        }
    }
}

int main()
{
    init();
    printf("Server initializes\n");
    while(1)
    {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int fd = accept(sockfd_des, (struct sockaddr*)&client, &len);
        if(fd == -1)
        {
            printf("Client occurs error...\n");
            continue;
        }
        int i;
        for(i=0;i<size;i++)
        {
            if(fds[i]==0)
            {
                fds[i] = fd;//record fd
                pthread_t tid;
                pthread_create(&tid, 0, service_thread, &fd);//tid會存放此pthread的tid，3rd argument:void*()，4th:3rd's func argument
                printf("fd=%d\n",fd);
                break;
            }
        }
        if(i == size)// full
        {
            char *str = "Sorry, the room is full now,please join later\n";
            send(fd, str, strlen(str), 0);
            close(fd);
        }
    }
}