#include "csapp.h"

void doit(int fd);
void optDoit(int fd);
void echoDoit(int fd);
void clienterror(int fd,char* cause,char* errnum ,char* shortmsg,char* longmsg);
void read_requesthdrs(rio_t* rp);
int parse_uri(char* uri,char* filename,char* cgiargs);
int server_static(int fd,char* filename,int filesize);

int main(int argc,char** argv)
{
    int listenfd,connfd;
    char hostName[MAXLINE],port[MAXLINE];       //the storge of client information
    socklen_t clientlen;                        //the length of client socket
    struct sockaddr_storage clientaddr;         //the address of client

    if(argc!=2)
    {
        fprintf(stderr,"usage: %s <port>\n",argv[0]);       //did`t used port num
        exit(1);
    }

    listenfd=open_listenfd(argv[1]);            //used port to create listen fd
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd,(SA*)&clientaddr,&clientlen);
        Getnameinfo((SA*)&clientaddr,clientlen,hostName,MAXLINE,port,MAXLINE,0);
        printf("Accepted connection from (%s, %s)\n",hostName,port);
        //doit(connfd);
        optDoit(connfd);
        Close(connfd);
    }
}

void doit(int fd)
{
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
    char filename[MAXLINE],cgiargs[MAXLINE];
    rio_t rio;

    /*read request line and headers*/
    Rio_readinitb(&rio,fd);         //read informations from client fd
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;//read informations from rio to locla buf
    sscanf(buf , "%s %s %s",method,uri,version);    //parse the header of client input

    Rio_writen(fd,buf,strlen(buf));

    if(strcasecmp(method,"GET"))    //compare two strings ignore case only return 0 when equal
    {
        //block non-get request
        clienterror(fd,method,"501","Not Implemented",
                    "Tiny could not implement this method");
        return ;
    }
    read_requesthdrs(&rio);         //read informations from client fd,ignore the header

    /* for a parse_uri function
    *   translate a uri and the next two parameter to store the result of parsing
    *   the function return 1 when the request is static
    *   return 0 when the request is dynamic
    */
    is_static = parse_uri(uri,filename,cgiargs);    //parse the uri

    //uri referance to a file on server,to detection if it exist
    /* the stat return 0 only exact find the file
     * when success if finding the file, system will write content to second argument
    */
    if(stat(filename,&uri)<0)
    {
        clienterror(fd,filename,"404","Not found","Tiny could not find this file");
        return ;
    }

    //struct stat sbuf;
    if(is_static)
    {
        //S_ISREG is a Macro ,used to dected whether the file is a regular file
        //st_mode is a field symbol the file type and permissions
        //S_IRUSR&sbuf.st_mode dected whether the server has the permission to visit the file
        if (!(S_ISREG(sbuf.st_mode)) || !(sbuf.st_mode & S_IRUSR))
        {
            clienterror(fd,filename,"403","Forbidden","tiny could not read the file");
            return ;
        }
        server_static(fd,filename,sbuf.st_size);
    }
    else        //a dynamic request , want to request a progress
    {
        //S_IXUSR & sbuf.st_mode will used & to detect if the file is valid for this server
        if(!(S_ISREG(sbuf.st_mode))||!(S_IXUSR & sbuf.st_mode))
        {
            clienterror(fd,filename,"403","Forbidden","tiny could not run the CGI program");
            return ;
        }
        server_dynamic(fd,filename,cgiargs);
    }
}

//this function used to deal with some simpy problems
/* fd ref to the client,cause ref to a simpy explain of where error
    errnum ref to the html error num
    the next two msg mean some simpy msg of error
*/
void clienterror(int fd,char* cause,char* errnum ,char* shortmsg,char* longmsg)
{
    char buf[MAXLINE], body[MAXBUF];
 
	/* Build the HTTP response body */
    //generate the body send to client
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\bn", body);
	sprintf(body, "%s%s: %s\r\n",body, errnum, shortmsg);
	sprintf(body, "%s<p>%s:%s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
 
	/* Print the HTTP response */
	sprintf(buf, "HTTP 1.0 %s %s \r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
    //specify the return type
	sprintf(buf, "Content-type:text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
    //specify the body length,do not forget a empty line
	sprintf(buf, "Content-Length:%d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
    //send body to fd behind the html body
	Rio_writen(fd, body, strlen(body));

    //after the action, the return fd is generate successfully,then need to send back
}

//read data from rp,which ref to a struct storge client data
//ignore the request headers by Rio
void read_requesthdrs(rio_t* rp)
{
    char buf[MAXLINE];
 
    //read client fd line by line ,stop when encounter the \r\n
	Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    #if 1
	while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    #endif

	return;
}

//the function parse the uri and storge in the next two arguments
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char* ptr;
    //strstr return a pointer ref to first loc the substring appear
    //the corresponding corret status should be to return 0
    if(!strstr(uri,"cgi_bin"))
    {
        strcpy(cgiargs,"");         //refresh the cgiargs,do not need it
        strcpy(filename,".");       //set the father file loc
        strcat(filename,uri);       //connect the filename and uri
        if(uri[strlen(uri)-1]=='/')
        {
            //set the default action when the request end with '/'
            strcat(filename,"home.html");
        }
        //return 1 mean this uri is a static request
        return 1;
    }
    else{
        //to search the loc of the ?
        ptr=strchr(uri,'?');
        if(ptr)
        {
            strcpy(cgiargs,ptr+1);
            *ptr='\0';
        }
        //non-argements to pass
        else{
            strcpy(cgiargs,"");
        }
        strcpy(filename,".");
        strcat(filename,uri);
        return 0;
    }
}

//provide static resources function
int server_static(int fd,char* filename,int filesize)
{
    int srcfd;
    char* srcp,filetype[MAXLINE],buf[MAXLINE];

    //generate the reponse body
    //auto generate the reponse file type
    get_filetype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");                    //200 ref to success
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length:%d\r\n", buf, filesize);
	sprintf(buf, "%sContent_type:%s\r\n\r\n", buf, filetype);//do not forget a \r\n
	Rio_writen(fd, buf, strlen(buf));       //wirte into the reponse fd
 
	/* Send response body to client */
	srcfd = Open(filename, O_RDONLY, 0);
	srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
	Close(srcfd);
	Rio_writen(fd, srcp, filesize);
	Munmap(srcp, filesize);         //release fd map
}

//auto generate the suffix by the kind of filename type
void get_filetype(char* filename,char* filetype)
{
    if(strstr(filename,".html"))
    {
        strcpy(filetype,"text/html");
    }
    else if (strstr(filename, ".gif"))
	{
		strcpy(filetype, "image/gif");
	}
	else if (strstr(filename, ".jpg"))
	{
		strcpy(filetype, "image/jpeg");
	}
	else
	{
		strcpy(filetype, "text/plain");
    }
}

void server_dynamic(int fd,char* filename,char* cgiargs)
{
    char buf[MAXLINE],*emptylist[]={ };

    /* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server:Tiny web Server \r\n");
	Rio_writen(fd, buf, strlen(buf));
    //there is non "Content-Length",the CGI will auto provide it
 
	if (Fork() == 0) /* child */
	{
		/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1);
        //redirect output to client fd
		Dup2(fd, STDOUT_FILENO);
		Execve(filename, emptylist, environ);
	}
    //father will wait until child finish
 	Wait(NULL);
}

void echoDoit(int fd) {
    char buf[MAXLINE];
    rio_t rio;
    ssize_t n;

    Rio_readinitb(&rio, fd);
    // 读取一行或直到连接关闭
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) > 0) {
        //printf("Server received: %s", buf);
        Rio_writen(fd, buf, n);  // 回显相同内容
    }
}

void optDoit(int fd) {
    char buf[MAXLINE];
    rio_t rio;

    // 初始化Rio并读取第一行数据
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE)) {
        printf("unknown error!!!\n");
        return;  // 读取失败直接返回
    }

    // 判断是否是HTTP请求（例如是否包含 "HTTP" 关键字）
    if (strstr(buf, "HTTP")) {
        // 如果是HTTP请求，调用原有逻辑
        doit(fd);
    } else {
        // 否则调用回显逻辑
        // 注意：需要将已读取的第一行数据写回
        Rio_writen(fd, buf, strlen(buf));
        echoDoit(fd);
    }
}