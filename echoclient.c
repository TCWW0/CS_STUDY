#include "csapp.h"

int main(int argc,char** argv)
{
    int clientfd;
    char* host,*port,buf[MAXLINE];
    rio_t rio;

    host=argv[1];
    port=argv[2];

    clientfd=Open_clientfd(host,port);
    Rio_readinitb(&rio,clientfd);

    while (Fgets(buf,MAXLINE,stdin)!=NULL)
    {
        Rio_writen(clientfd,buf,strlen(buf));
        ssize_t n =Rio_readlineb(&rio,buf,MAXLINE);
        if (n <= 0) {
            printf("Server closed connection.\n");
            break;  // 服务器关闭连接，退出循环
        }
        Fputs(buf,stdout);
    }
    close(clientfd);
    exit(0);
}