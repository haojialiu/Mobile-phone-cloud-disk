#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<termios.h>
#include<sys/ioctl.h>

#define ARGC 50
/*struct winsize
{
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};*/

void get_file(char *read_buff)
{
    int size_col = get_col();
    char *file_buff[ARGC] = {0};
    int i = 0;
    int num = 0;
    int file_max = 0;
    char *str = strtok(read_buff,"\n");
    file_buff[i++] = str;
    while((str=strtok(NULL,"\n")) != NULL)
    {
        file_buff[i++] = str;
    }
    i--;
    int ii = 0;
    while(ii<=i)
    {
       int max = strlen(file_buff[ii++]);
       if(max > file_max)
       {
            file_max = max;   
        }
    }
    num = size_col / (file_max + 3);
    int file_num = size_col / num;
    /*printf("%d\n",num);
    printf("%d\n",file_num);
    printf("%d\n",size_col);*/
    int m = 0;
    while(m <= i)
    {
        int n = num;
        while(n > 0 && m<=i)
        {

            int file_num1 = file_num - strlen(file_buff[m] );
            printf("%s",file_buff[m++]);
           while(file_num1 > 0)
            {
                printf(" ");
                file_num1--;
            }
            n--;
        }
        printf("\n");
    }
}
int get_col()
{
    struct winsize size;
    ioctl(STDOUT_FILENO,TIOCGWINSZ,&size);
//    printf("%d\n",size.ws_col);
    return size.ws_col;
}

void get_md5(int sockfd,char * name)
{
    char * m_crr[128] = {0};
     m_crr[1] = name;
     m_crr[0] = "md5sum";
    int pipfd[2];
    pipe(pipfd);
    pid_t pid = fork();
    if(pid == 0)
    {
        dup2(pipfd[1],1);
        execvp(m_crr[0],m_crr);
        exit(0);
    }
    close(pipfd[1]);
    wait(NULL);
   char cli_crr[128] = {0};
    read(pipfd[0],cli_crr,40);
    printf("%s",cli_crr);
    fflush(stdout);
 /*   char ser_crr[128] = {0};
    recv(sockfd,ser_crr,40,0);
    printf("%s",ser_crr);
    if(strcmp(ser_crr,cli_crr))
    {
        printf("difference\n");
    }
    else
    {
        printf("equal\n");
    }*/
}

void send_file(int sockfd,char* name)
{
    int fd = open(name,O_RDONLY);
    if( fd == -1)
    {
        printf("not found!");
        return;
    }

    int size = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);

    char status[32] = {0};
    sprintf(status,"ok#%d",size);
    send(sockfd,status,strlen(status),0);
    printf("file(%s):%d\n",name,size);

    char ser_status[32] = {0};
    if(recv(sockfd,ser_status,31,0) <= 0)
    {
        return;
    }
    if(strcmp(ser_status,"ok") != 0)
    {
        return;
    }

    char data[256] = {0};
    int num = 0;
    int cur_size = 0;
    while((num = read(fd,data,256)) > 0)
    {
        send(sockfd,data,num,0);
        cur_size = cur_size + num;
        float f = cur_size * 100.0 / size;
        printf("upload:%.2f%%\r",f);
        fflush(stdout);
    }
    printf("\n");
    
    get_md5(sockfd,name);
    char ser_crr[128] = {0};
    recv(sockfd,ser_crr,40,0);
    printf("%s",ser_crr);
    close(fd);
    return;
}

int recv_file(int sockfd,char* name)
{
    char buff[128] = {0};
    if(recv(sockfd,buff,127,0) <= 0)
    {
        return -1;
    }
    if( strncmp(buff,"ok",2) != 0)
    {
        printf("%s\n",buff);
        return 0;
    }

     int size = 0;
     sscanf(buff+3,"%d",&size);

     printf("file(%s):%d\n",name,size);
    printf("%s",buff+5);
     int fd = open(name,O_WRONLY|O_CREAT,0600);
     if( fd == -1 )
     {
        send(sockfd,"err",3,0);
        return;
     }
     send(sockfd,"ok",2,0);

     int num = 0;
     int cur_size = 0;
     char data[256] = {0};
     while(1)
     {
         num = recv(sockfd,data,256,0);
         if( num <= 0)
         {
             return -1;
         }

         write(fd,data,num);
         cur_size = cur_size + num;
         float f = cur_size * 100.0 / size;
         printf("download:%.2f%%\r",f);
         fflush(stdout);
         if(cur_size >= size)
         {
             break;
         }
     }

    get_md5(sockfd,name);
     printf("\n");
 //    return 0;
}
int main()
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd != -1);

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6000);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    assert(res != -1);

    while(1)
    {
        char buff[128] = {0};
        printf("Connect success ~ ]$ ");
        fflush(stdout);
        fgets(buff,128,stdin);

        if(strncmp(buff,"end",3)==0)
        {
            break;
        }
        buff[strlen(buff)-1] = 0;

        if(buff[0] == 0)
        {
            continue;
        }

        char tmp[128] = {0};
        strcpy(tmp,buff);

        char* myargv[10] = {0};
        char*s = strtok(tmp," ");

        int i = 0;
        while(s != NULL)
        {
            myargv[i++] = s;
            s = strtok(NULL," ");
        }

        if(strcmp(myargv[0],"get") == 0)
        {
            send(sockfd,buff,strlen(buff),0);
            recv_file(sockfd,myargv[1]);
        }
        else if(strcmp(myargv[0],"put") == 0)
        {
            send(sockfd,buff,strlen(buff),0);
            send_file(sockfd,myargv[1]);
        }
        else
        {
            send(sockfd,buff,strlen(buff),0);

            char read_buff[1024] = {0};
            recv(sockfd,read_buff,1023,0);
      //      get_col();
            get_file(read_buff);            
     //       printf("%s",read_buff);
        }
    }
    close(sockfd);
}
