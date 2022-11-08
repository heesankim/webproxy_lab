#include <stdio.h>
#include "csapp.h"
#include <stdlib.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *proxy_host;
static const char *proxy_port;

void doit(int fd);
void read_requesthdrs(rio_t *rp);
make_request_to_server(int fd, char *url, char *host, char *port, char *method, char *version, char *filename);
void parsing(char *host, char *uri, char *filename, char *port);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
  int listenfd, connfd;                  //서버에서 만드는 소켓
  char hostname[MAXLINE], port[MAXLINE]; // 주소/IP, port
  socklen_t clientlen;                   // 클라이언트의 ip = 32비트
  struct sockaddr_storage clientaddr;    //클라이언트소켓주소

  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]); // error메시지를 저장한다음에 출력하고 종료
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]); // 듣기식별자 생성

  while (1)
  {
    clientlen = sizeof(clientaddr);              //클라이언트주소 크기
    connfd = Accept(listenfd, (SA *)&clientaddr, //클라이언트의 연결을 기다리고 받아주는 것 -> 연결식별자가 리턴( 0보다 크거나 같음 ) / -1 출력되면 에러
                    &clientlen);                 // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);                                                // 리턴값이 0 / 잘 안 되면 에러코드
    printf("Accepted connection from (%s, %s)\n", hostname, port); // 클라이언트 Ip와 port 출력
    doit(connfd);                                                  // line:netp:tiny:doit
    Close(connfd);                                                 // line:netp:tiny:close
  }

  printf("%s", user_agent_hdr);
  return 0;
}

void doit(int fd)
{
  struct stat sbuf; // 소켓버프 (임시저장변수)
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], url[MAXLINE], host[MAXLINE], port[MAXLINE], version[MAXLINE], filename[MAXLINE];
  //예를 들어 uri는 파일이름과 인자, 버젼은 http 1.0 / 1.1
  char rebuf[MAX_OBJECT_SIZE]; // cgi인자는 ?뒤에 나오는 것으로 &로 구분
  rio_t client_rio, pts_rio;

  /* Read request line and headers */
  Rio_readinitb(&client_rio, fd);           // connectfd와 rio버퍼를 연결해주고 fd에 있는 값들을 전달해준다
  Rio_readlineb(&client_rio, buf, MAXLINE); // rio버퍼에 있는 것을 buf에다가 복사해준다.
  printf("Request headers:\n");
  printf("%s\n", buf); // buf에는 method, uri, version이 띄어쓰기로 나열되어 있다!!!

  sscanf(buf, "%s %s %s", method, uri, version); // method / uri / version 정의 완료

  parsing(host, uri, filename, port);

  if (strcasecmp(method, "GET")) // 인자 1과 인자2가 같으면 0을 출력합니다!
  {
    sprintf(buf, "GET요청이 아닙니다. \r\n");
    Rio_writen(fd, buf, strlen(buf));
    return;
  }

  int ptsfd = Open_clientfd(host, port);
  Rio_readinitb(&pts_rio, ptsfd);

  read_requesthdrs(&client_rio);
  make_request_to_server(ptsfd, url, host, port, method, version, filename);

  Rio_readnb(&pts_rio, rebuf, MAX_OBJECT_SIZE);
  printf("%s\n", rebuf);

  Rio_writen(fd, rebuf, MAX_OBJECT_SIZE);

  printf("Response headers:\n");

  printf("%s", rebuf);
  Close(ptsfd);
}

int make_request_to_server(int fd, char *url, char *host, char *port, char *method, char *version, char *filename)
{
  char buf[MAXLINE];

  if (strlen(filename) == 0)
  {
    strcpy(url, "/\n");
  }
  else
  {
    strcpy(url, "/");
    strcat(url, filename);
  }

  // printf("++++++++++++++++++ is %s ++++++++++++++++", url);

  sprintf(buf, "%s %s %s\r\n", method, url, version);
  sprintf(buf, "%sHost: %s:%s\r\n", buf, host, port);
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);
  Rio_writen(fd, buf, strlen(buf));
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); // rp에서 텍스트를 읽고 buf로 복사
  printf("%s", buf);

  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }

  return;
}

void parsing(char *host, char *uri, char *filename, char *port)
{

  char *p;

  if (p = strchr(uri, '/'))
  {
    *p = '\0';
    sscanf(p + 2, "%s", host);
  }

  else
    strcpy(host, uri);

  if ((p = strchr(host, ':')))
  {
    *p = '\0';
    sscanf(host, "%s", host);
    sscanf(p + 1, "%s", port);
  }

  if ((p = strchr(port, '/')))
  {
    *p = '\0';
    sscanf(port, "%s", port);
    sscanf(p + 1, "%s", filename);
  }
}
