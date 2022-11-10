/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"
#include <signal.h>

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);


int main(int argc, char **argv) 
{
  // 듣기 식별자, 연결 식별자 선언
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 듣기 식별자 할당
  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    // 연결 식별자 생성, 할당
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("<------------start of request------------->\n");
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close

  }

}

void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  // initialize Rio_read
  Rio_readinitb(&rio, fd);

  // Robustly read a text line (buffered)
  // buf에 읽은 데이터가 저장된다. (포인터 작업 수행됨.)
  // 맨 위 한줄(=request line)을 읽는다.
  Rio_readlineb(&rio, buf, MAXLINE);

  printf("Request line:\n");
  printf("%s", buf);
  // request line에서 method, uri, version 값을 split해서 받는다.
  sscanf(buf, "%s %s %s", method, uri, version);

  // string case compare 함수: 대소문자를 구분하지 않고 두 string의 철자와 길이를 비교한다. 
  // 두 string이 같으면 0을 리턴하고 / 1번 스트링이 더 짧으면 음수 리턴 /2번 스트링이 더 짧으면 양수 리턴.

  // 따라서 method가 GET이 나오면 0을 리턴 받고 아래의 if문은 들어가지 않고 넘어간다.
  // method가 GET이 아닌 다른 것이 들어오면 1을 리턴받고 if문에 들어간다.
  if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")){
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  // read_requestheaders
  // request line 아래의 정보들(request header)을 모두 읽고 버퍼에 저장한다.
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  // 정적 컨텐츠인지 아닌지 검사한다.
  is_static = parse_uri(uri, filename, cgiargs);
  

  // stat -> gets status information about a specified file, places it in the area of memory pointed to by the buf argument.
  // returns 0 if successful and -1 if not.
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }


   /* Serve static content */
  //  정적 콘텐츠라면
  if (is_static) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    // printf("%s", filename);
    serve_static(fd, filename, sbuf.st_size, method);
    
  }
  /* Serve dynamic content */
  // 동적 콘텐츠라면
  else{
    printf("이 컨텐츠는 동적 컨텐츠 입니다.\n");
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs, method);
  }
}


// 클라이언트 에러 메세지 출력
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // sprintf(): // 형식화된 데이터를 버퍼로 출력한다.
  // Rio_writen(): Robustly write n bytes

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));

}

// request header를 읽는다.
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  // "carriage return" "line feed" -> "\r\n"
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }

  return;
}

// uri의 문자열을 검사하여 정적 컨텐츠인지 아닌지 검사한다.
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // strstr 함수: string1에서 string2의 첫 번째 표시를 찾는다.
  // uri 변수에서 "cgi-bin"을 찾지 못한 경우,
  // 정적컨텐츠인 경우이다.
  if (!strstr(uri, "cgi-bin")) {
    // strcpy 함수: string2를 string1에서 지정한 위치로 복사한다.
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    // strcat 함수: string2를 string1에 연결하고 널 문자로 결과 스트링을 종료.
    // filename -> ./
    strcat(filename, uri);

    // 최소한의 URL을 갖은 경우 -> 특정 기본 홈페이지("home.html")로 확장한다.
    if (uri[strlen(uri)-1] == '/') {
      strcat(filename, "home.html");
    }

    return 1;
  }
  // 정적 컨텐츠가 아닌 경우 
  else{

    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }else{
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);

    return 0;
  }

}

// 정적 콘텐츠를 serve.
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];


  /* Send response headers to client */
  get_filetype(filename, filetype);
  // response line
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // response header
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  // "\r\n\r\n"으로 표시해서 마지막 빈 줄도 넣어준다.
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  // 작성한 내용을 저장.
  Rio_writen(fd, buf, strlen(buf));

  // 저장된 response header를 출력.
  printf("Response headers:\n");
  printf("%s", buf);

  if (!strcasecmp(method, "HEAD")){
    return;
  }

  /* Send response body to client */
  // filename을 open할 수 있는지 확인.
  srcfd = Open(filename, O_RDONLY, 0);

  // Mmap()이 요청한 파일을 가상 메모리 영역으로 매핑한다.
  // srcfd의 첫번째 filesize 바이트를 주소 srcp에서 시작하는 사적 읽기-허용 가상메모리 영역으로 매핑한다.
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  srcp = (int *)malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  // srcfd 식별자는 더이상 필요 없으므로 닫는다.
  Close(srcfd);
  // srcp주소에 filesize메모리를 저장한다.
  Rio_writen(fd, srcp, filesize);
  // 매핑된 가상 메모리 주소를 반환한다.
  // Munmap(srcp, filesize);
  free(srcp);

}


void response_head(int fd, char *filename, int filesize){
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];


  /* Send response headers to client */
  get_filetype(filename, filetype);
  // response line
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // response header
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  // "\r\n\r\n"으로 표시해서 마지막 빈 줄도 넣어준다.
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  // 작성한 내용을 저장.
  Rio_writen(fd, buf, strlen(buf));

  // 저장된 response header를 출력.
  printf("Response headers:\n");
  printf("%s", buf);
}


/*
get_filetype - Derive file type from filename
*/
// filename의 문자열을 검사해서 확장자를 알아낸다.
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html")) {
    strcpy(filetype, "text/html");
  }
  else if (strstr(filename, ".gif")) {
    strcpy(filetype, "image/gif");
  }
  else if (strstr(filename, ".png")) {
    strcpy(filetype, "image/png");
  }
  else if (strstr(filename, ".jpg")) {
    strcpy(filetype, "image/jpeg");
  }
  else if (strstr(filename, ".mp4")) {
    strcpy(filetype, "video/mp4");
  }
  else {
    strcpy(filetype, "text/plain");
  }
}



void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));


  if (Fork() == 0) { /* Child */
    /* Real server would set all CGI vars here */
    setenv("QUERY_STRING", cgiargs, 1);
    setenv("REQUEST_METHOD", method, 1);
    Dup2(fd, STDOUT_FILENO); /* Redirect stdout to client */
    Execve(filename, emptylist, environ); /* Run CGI program */
  }

  Wait(NULL); /* Parent waits for and reaps child */
}
