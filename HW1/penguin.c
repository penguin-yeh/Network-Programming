#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <strings.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>

void sigchld(int signo)
{
	pid_t pid;
	
	while((pid=waitpid(-1, NULL, WNOHANG))>0);
}

int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;

 while ((i < size - 1) && (c != '\n'))
 {
    n = recv(sock, &c, 1, 0);
    /* DEBUG printf("%02X\n", c); */
    if(n > 0)
    {
        if (c == '\r')
        {
            n = recv(sock, &c, 1, MSG_PEEK);
            /* DEBUG printf("%02X\n", c); */
            if ((n > 0) && (c == '\n'))
                recv(sock, &c, 1, 0);
            else
                c = '\n';
        }
        buf[i] = c;
        i++;
    }
    else
        c = '\n';
    }
    buf[i] = '\0';
    return(i);
}

int handle_socket(int new_socket)
{
    char message[100000];
    long ret;
    int numchars = 1;
    int content_length = 0;
    int idx = 0;
    char type[1024];
    char file_name[1024];
    char link[1024];
    char *temp;
    int des_fd;
    int pid;
    
    numchars = get_line(new_socket, message, sizeof(message));
    printf("%s", message);
    temp = strstr(message, "/");
    while(*temp!=' ')
    {
            link[idx++] = *temp;
            temp++;
    }
    if(strcmp(link,"/")==0)
    {
        strcpy(link, "index.html");
    }    
    if(link[0] == '/')
    {
        for(int i=0;i<strlen(link)-1;i++)
        {
            link[i] = link[i+1];
        }
        link[strlen(link)-1] = '\0';
    }
    printf("link:%s\n",link);
    if(strstr(message, "POST"))
        strcpy(type,"POST");
    else if(strstr(message, "GET"))
        strcpy(type, "GET");

    if(strcmp(type,"POST")==0)
    {
        printf("type:%s\n",type);
        des_fd = open("penguin.txt", O_RDWR | O_CREAT | O_TRUNC, 0700);
        printf("start\n");
        // int read_len = read(new_socket, message, 100000);
        // printf("%s\n",message);
        // return 0;
        while ((numchars > 0) && strcmp("\n", message))  /* read & discard headers */
        {
            numchars = get_line(new_socket, message, sizeof(message));
            //printf("%s",message);
            message[15] = '\0';
            if(strcasecmp(message, "Content-Length:") == 0)
            {
                content_length = atoi(&message[16]);
            }    
            if(temp=strstr(message, "filename"))
                strcpy(file_name, temp+strlen("filename")+2);
        }
        //numchars = get_line(new_socket, message, sizeof(message));
        // while ((numchars > 0) && strcmp("\n", message))  /* read & discard headers */
        // {
        //     numchars = get_line(new_socket, message, sizeof(message));
        //     if(temp=strstr(message, "filename"))
        //         strcpy(file_name, temp+strlen("filename")+2);
        // }           
        numchars = get_line(new_socket, message, sizeof(message));
        while((numchars > 0) && strcmp("\n",message))
        {
            //printf("%s",message);
            if(temp=strstr(message, "filename"))
                strcpy(file_name, temp+strlen("filename")+2);
            numchars = get_line(new_socket, message, sizeof(message));
        }
        for(int i=0;i<strlen(file_name);i++)
        {
            if(file_name[i]=='"')
                file_name[i]='\0';
        }
        //printf("%s\n", message);
        //printf("end\n");
        printf("name:%s\n",file_name);
        printf("length = %d\n",content_length);
        // get file size

        // char *file_size = strstr(message, "Content-Length:");
        // int real_file_size = atoi(file_size + 16);
        // printf("%d\n", real_file_size);
        // printf("real : %d\n",strlen(message));
	//memset(message, 0x00, sizeof(message));

        if (!des_fd)
        {
            perror("dest_fd error : ");
        }
        char tmp[20000];
        int cont;
        int num = 0;
        int counter = 0;
        //numchars = get_line(new_socket, message, sizeof(message));
	//printf("%s",message);
        while (counter < content_length)
        {
            idx = 0;
            cont = read(new_socket, message, 100000);
            memset(tmp,0,sizeof(tmp));
            //printf("%d ",cont);
            counter += cont;
            printf("%d ",counter);
            //printf("%s ", message);
            //write(des_fd, message, numchars);
            // numchars = get_line(new_socket, message, sizeof(message));
            //             printf("%d ",numchars);
            // printf("%s ", message);
            for(int i=0;i<cont;i++)
            {
                if(message[i]=='-')
                {
		            tmp[idx++] = message[i];
                    num++;
                }
                else 
                {
                    tmp[idx++] = message[i];
                    num = 0;
                }
                if(num==5)
                    break;
            }
            if(num==5)
            {
		        idx -= 5;
                for(int i=idx-1;i>0;i--)
                {
                    if(tmp[i]=='\n' && tmp[i-1]=='\r')
                    {
                        printf("!!!!!!\n");
                        //tmp[i-1] = '\0';
			            idx -= 2;
                        break;
                    }
                }
		//printf("%s", tmp);
                write(des_fd, tmp, idx);
                break;
            }
            else
            {
                //printf("%s", tmp);
                write(des_fd, tmp, cont);
            }
            // if(num==5)
            //     break;
            //write(des_fd, tmp, cont);

            // printf("ret: %d\n", ret);
            // counter += ret;

            // if (counter == real_file_size)
            //     break;
            // break;
        }
        printf("done!!!\n");
        //Reply to the client
        //strcpy(message ,"Hello Client , I have received your connection. But I have to go now, bye\n");

        char header[1000];
        int file_fd;
        printf("here\n");
        if ((file_fd = open("success.html", O_RDONLY)) == -1)
            write(new_socket, "Failed to open file", 19);
        else
            printf("open success!!\n");
        off_t size = lseek(file_fd, 0, SEEK_END);
        lseek(file_fd, 0, SEEK_SET);
        printf("hello!!!!\n");
        sprintf(header, "Content-length: %d\n", size);

        write(new_socket, "HTTP/1.1 200 OK\n", strlen("HTTP/1.1 200 OK\n"));
        write(new_socket, header, strlen(header));
        write(new_socket, "Content-Type: text/html\n\n", 25);
        // strcpy(message ,"我好帥");
        // sprintf(message, "<html><head><meta charset=%s></head>我好帥</html>", "utf8");
        //write(new_socket, message, strlen(message));
        int ret;
        while ((ret = read(file_fd, message, 8000)) > 0)
        {
            write(new_socket, message, ret);
        }

        char cmd[102400];
        sprintf(cmd, "mv ./penguin.txt ./%s", file_name);
        system(cmd);

        //update file_html
        char buff[1024];
        sprintf(buff, "<a href=\"/%s\">%s</a><br><br>", file_name, file_name);
        //printf("buffer:%s\n",buff);
        file_fd = open("file.html",O_APPEND | O_RDWR,777);
        write(file_fd, buff, strlen(buff));
        close(file_fd);
        close(new_socket);
    }
    else if(strcmp(type,"GET")==0)
    {
        char header[1000];
        int file_fd;
        printf("type:%s\n",type);
        while ((numchars > 0) && strcmp("\n", message))  /* read & discard headers */
        {
            numchars = get_line(new_socket, message, sizeof(message));
            printf("%s",message);
        }    
        if ((file_fd = open(link, O_RDONLY)) == -1)
            printf("Failed to open\n");
        else
            printf("open index!!\n");
        off_t size = lseek(file_fd, 0, SEEK_END);
        lseek(file_fd, 0, SEEK_SET);
        printf("hello~~\n");
        sprintf(header, "Content-length: %d\n", size);

        write(new_socket, "HTTP/1.1 200 OK\n", strlen("HTTP/1.1 200 OK\n"));
        write(new_socket, header, strlen(header));
        if(strcmp(link,"index.html")!=0 && strcmp(link,"favicon.ico")!=0 && strcmp(link, "file.html")!=0 && strcmp(link, "button.css")!=0)
        {
            char header[1024];
            sprintf(header, "Content-Disposition: attachment; filename=\"%s\"\n", link);
            write(new_socket, header, strlen(header));
        }    

        if(strcmp(link,"favicon.ico")==0)
            write(new_socket, "Content-Type: image/x-icon\n\n", 28);
        else
            write(new_socket, "Content-Type: text/html\n\n", 25);//css also

        int ret;
        while ((ret = read(file_fd, message, 8000)) > 0)
        {
            write(new_socket, message, ret);
        }
        // strcpy(message ,"我好帥");
        // sprintf(message, "<html><head><meta charset=%s></head>我好帥</html>", "utf8");
        // write(new_socket, message, strlen(message));

        close(new_socket);
        if (new_socket < 0)
        {
            perror("accept failed");
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int socket_desc, new_socket, c;
    struct sockaddr_in server, client;

    srand(time(NULL));//for random_port

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    int random_port = rand() % 60000;
    server.sin_port = htons(random_port);
    int pid;

    //Bind
    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("bind failed");
        return 1;
    }
    puts("bind done");
    printf("bind to %d port\n", random_port);

    listen(socket_desc, 5);
    signal(SIGCHLD, sigchld);

    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c))!=-1)
    {
        puts("Connection accepted");

        // int read_len = read(new_socket, message, 100000);
        // printf("%s\n",message);
        // continue;
        if((pid = fork()) < 0)//fail
        {
            exit(0);
        }
        else
        {
            if(pid == 0)//child
            {
                close(socket_desc);
                handle_socket(new_socket);
                exit(0);
            }
            else//parent
            {
                close(new_socket);
            }
        }
    }
    return 0;
}
       
