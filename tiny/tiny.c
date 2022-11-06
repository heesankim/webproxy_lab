/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

// 타이니는 포트로부터 오는 연결 요청을 반복적으로 듣기 수행하는 서버임
int main(int argc, char **argv)
{
  int listenfd, connfd; //듣기소켓, 연결소켓 초기화
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {

    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // Open_listenfd 함수는 4가지의 일을 함 ( 각각의 함수 )
  // listen 식별자를 리턴해서 listenfd에 넣는다.
  // 함수 호출 듣기 소켓을 오픈함. 클라이언트의 정보가 담긴다.
  listenfd = Open_listenfd(argv[1]);
  // ./tiny 5000 이렇게 주면
  // argv[0]에 ./tiny 들어가고 argv[1]에 5000 들어감

  while (1)
  {
    clientlen = sizeof(clientaddr);

    // Accept 실행 > 연결 받음 > 트랜잭션 수행 > 연결 닫음. (계속해서 반복)
    // accept함수를 통해 연결 요청한 clientfd와 연결
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    // client에서 받은 요청을 처리하는 doit 함수 진행
    //  한 개의 HTTP 트랜잭션을 처리
    //  요청 받은 데이터를 클라이언트에게 보내줌
    doit(connfd);

    Close(connfd);
  }
}

void doit(int fd) // tiny doit 은 한 개의 HTTP 트랜잭션을 처리한다.
{
  int is_static; // 정적이냐 동적이냐를 나타내는 값이 담김.
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers*/
  Rio_readinitb(&rio, fd); //
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);

  if (strcasecmp(method, "GET"))
  {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");

    return;
  }

  read_requesthdrs(&rio);

  is_static = parse_uri(uri, filename, cgiargs); // 요청이 동적인지 정적인지 플래그를 설정한다.
  // cgiargs 는 동적 컨텐츠의 실행 파일에 들어갈 인자
  if (stat(filename, &sbuf) < 0)
  {
    // 파일이 디스크 상에 없다면 에러메시지
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");

    return;
  }

  if (is_static) /* Serve static content */
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) // 읽기 권한 검증
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }

    serve_static(fd, filename, sbuf.st_size); // 읽기 권한 통과시 정적컨텐츠 제공
  }

  else /* Serve dynamic content */
  {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) // 읽기 권한 검증
    {
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }

    serve_dynamic(fd, filename, cgiargs); // 읽기 권한 통과시 동적컨텐츠 제공
  }
}

void read_requesthdrs(rio_t *rp)
// tiny 웹서버는 요청 헤더 내의 어떤 정보도 사용하지 않음
// 따라서 요청 헤더를 종료하는 빈 텍스트줄(\r\n)이 나올 때까지 요청 헤더를 모두 읽어들임
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);

  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }

  return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  if (!strstr(uri, "cgi-bin"))
  {
    strcpy(cgiargs, "");

    strcpy(filename, ".");
    strcat(filename, uri);

    if (uri[strlen(uri) - 1] == '/')
      strcat(filename, "home.html");

    return 1;
  }

  else
  {
    ptr = index(uri, '?');
    if (ptr)
    {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0'; // \0 은 NULL 값임
    }
    else
      strcpy(cgiargs, "");

    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  get_filetype(filename, filetype);

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server \r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf)); // 얘대신 딴거 써라 malloc, rio_readn, and rio_writen,

  printf("Response headers:\n");
  printf("%s", buf);

  srcfd = Open(filename, O_RDONLY, 0);
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // malloc, rio_readn, and rio_writen,
  srcp = (char *)malloc(sizeof(char) * filesize);

  rio_readn(srcfd, srcp, filesize);

  Close(srcfd);

  Rio_writen(fd, srcp, filesize); // 얘대신 딴거 써라malloc, rio_readn, and rio_writen,

  // Munmap(srcp, filesize);  // 얘대신 딴거 써라 malloc, rio_readn, and rio_writen,
  // free는 포인터 주소값이 파라미터로 들어간다.
  free(srcp);
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  if (Fork() == 0)
  {

    setenv("QUERY_STRING", cgiargs, 1);

    Dup2(fd, STDOUT_FILENO);

    Execve(filename, emptylist, environ);
  }

  Wait(NULL);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
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