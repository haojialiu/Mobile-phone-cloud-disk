#include"work_thread.h"

#define ARGC 10
/*
 *启动线程
 */
void thread_start(int c)
{
    pthread_t id;
    pthread_create(&id,NULL,work_thread,(void*)c);
}

/*获取md5值*/
void get_md5(int c,char * name,char * ser_crr)
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
 //  char ser_crr[128] = {0};
    read(pipfd[0],ser_crr,40);
   // send(c,ser_crr,40,0);
    //printf("%s",ser_crr);
}

/*
 *获取命令
 */
void get_argv(char buff[],char*myargv[])
{
    int i = 0;
    char *str = strtok(buff," ");
    myargv[i++] = str;          //第一次获取的是命令

    while((str = strtok(NULL," ")) != NULL)
    {
        myargv[i++] = str;        //每次循环获取命令中的参数
    }

}

int recv_file(int c,char* myargv[])
{
    char buff[128] = {0};
    if(recv(c,buff,127,0) <= 0)
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

//     printf("file(%s):%d\n",myargv[1],size);

     int fd = open(myargv[1],O_WRONLY|O_CREAT,0600);
     if( fd == -1 )
     {
        send(c,"err",3,0);
        return;
     }
     send(c,"ok",2,0);

     int num = 0;
     int cur_size = 0;
     char data[256] = {0};
     while(1)
     {
         num = recv(c,data,256,0);
         if( num <= 0)
         {
             return -1;
         }

         write(fd,data,num);
        cur_size = cur_size + num;
         if(cur_size >= size)
         {
             break;
         }
     }
    char  ser_crr[50] = {0};
    get_md5(c,myargv[1],ser_crr);
    send(c,ser_crr,40,0);
     return 0;
}

/*
 *发送文件
 */
void send_file(int c, char * myargv[])
{
    if(myargv[1] == NULL)
    {
        send(c,"get:no file name!",17,0);
        return;
    }

    int fd = open(myargv[1],O_RDONLY);
    if( fd == -1)
    {
        send(c,"not found!",10,0);
        return;
    }
    int size = lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);

    char status[32] = {0};
    sprintf(status,"ok#%d#",size);
    //char  ser_crr[50] = {0};
   // get_md5(c,myargv[1],ser_crr);
    //printf("%s",ser_crr);
    //strcat(status,ser_crr);
    send(c,status,strlen(status),0);

   // get_md5(c,myargv[1]);
    char cli_status[32] = {0};
    if(recv(c,cli_status,31,0) <= 0)
    {
        return;
    }
    if(strcmp(cli_status,"ok") != 0)
    {
        return;
    }

    char data[256] = {0};
    int num = 0;

    while((num = read(fd,data,256)) > 0)
    {
        send(c,data,num,0);
    }
    close(fd);
    return;
}


/*
 *工作线程
 */
void * work_thread(void * arg)
{
    int c = (int)arg;

    //测试
    while(1)
    {
        char buff[128] = {0};

        int n = recv(c,buff,127,0);     //读取客户端发来的数据
        if(n<=0)
        {
            close(c);
            printf("one client over\n");
            break;
        }

//        int len = strlen(buff);
//        buff[len-1] = '\0';             //把读出来的字符串最后一位换行符去掉


        char * myargv[ARGC] = {0};      
        get_argv(buff,myargv);

        if(strcmp(myargv[0],"get")==0)
        {
            send_file(c,myargv);
     //       get_md5(c,myargv[1]);
        }
        else if(strcmp(myargv[0],"put") == 0)
        {
            recv_file(c,myargv);
        }
        else
        {

            int fd[2];          //开两个描述符用来给管道的读端和写端
            pipe(fd);           //创建管道
            pid_t pid = fork();
            if(pid == 0)                    //复制一个进程用来执行命令
            {               
                dup2(fd[1],1);              //把标准错误输出改为管道的写端
                dup2(fd[1],2);              
                execvp(myargv[0],myargv);        //把复制出的进程替换成命令的进程  
                perror("exec error");
                printf("%s",myargv[0]);
                exit(0);
            }
            else
            {
                close(fd[1]);
                wait(NULL);
                char brr[1024] = {0};
                if(read(fd[0],brr+strlen(brr),1000) == 0)
                {
                    strncpy(brr,myargv[0],strlen(myargv[0]));
                    strcat(brr,"  finish\n");
                    send(c,brr,strlen(brr),0);
                }
                else
                {
                    send(c,brr,strlen(brr),0);
                }
                close(fd[0]);
                wait(0);
            }
        }
    }
}           
