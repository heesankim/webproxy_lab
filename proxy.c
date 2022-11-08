// telnet 43.201.149.74 5000
// GET http://43.201.149.74:5000 HTTP/1.1
// curl -v --proxy "43.201.149.74:5000" "43.201.149.74:8000"

#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
//static const char *proxy_host="43.201.7.218";
static const char *proxy_port;

int main(int argc, char **argv)
{
  int listenfd, connfd; // 듣기소켓, 연결소켓 받아오기
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  if (argc != 2) // 에러검출
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  listenfd = Open_listenfd(argv[1]);
  proxy_port = argv[1];

  while (1)
  {
    clientlen = sizeof(clientaddr);
    // Accept 실행 > 연결 받음 > 트랜잭션 수행 > 연결 닫음. (계속해서 반복)
    // accept함수를 통해 연결 요청한 clientfd와 연결
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    // Getnameinfo : ip 주소를 도메인주소로 바꿔줌
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    // client에서 받은 요청을 처리하는 doit 함수 진행
    //  한 개의 HTTP 트랜잭션을 처리
    //  요청 받은 데이터를 클라이언트에게 보내줌
    doit(connfd);
    Close(connfd);
  }
}
void doit(int fd)
{
  // 프록시는 get요청만 받으면 됨. 정적인지 동적인지 몰라도됨 서버입장에서 프록시는 클라이언트 임. is_static 필요X
  struct stat sbuf; // (필요함? 일단 가져옴)
  // sbuf 에 메타데이터 정보가 저장됨 (필요함)
  //
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], url[MAXLINE], version[MAXLINE], port[MAXLINE], host[MAXLINE], filename[MAXLINE];
  rio_t client_rio;
  char *p;

  Rio_readinitb(&client_rio, fd);
  Rio_readlineb(&client_rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s\n", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  
  strcpy(url, uri);
  if ((p = strchr(uri,'/'))) {
    *p = '\0';
    sscanf(p+2, "%s", host);
  }
  else {
    strcpy(host, uri);
  }

  if ((p = strchr(host,':'))) {
    *p = '\0';
    sscanf(host, "%s", host);
    sscanf(p+1, "%s", port);
  }

  if ((p = strchr(port, '/'))) {
    *p = '\0';
    sscanf(port, "%s", port);
    sscanf(p+1, "%s", filename);
  }


  if(strcasecmp(method, "GET")) {
    sprintf(buf, "GET요청이 아닙니다.\r\n");
    Rio_writen(fd, buf, strlen(buf));
    return;
  }

  read_requesthdrs(&client_rio);
  make_request_to_server(url, host, port, method, version, filename);

}

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE); // 텍스트 줄을 rp에서 읽고 buf에 복사한 후 널 문자로 종료시킨다. 최대 MAXLINE - 1개의 바이트를 읽는다.
  printf("rio_readlineb = %s", buf);
  while(strcmp(buf, "\r\n")) { // 첫번째 매개변수(buf)와 두번째 매개변수('\r\n')가 같을 경우 0을 반환
    Rio_readlineb(rp, buf, MAXLINE);
    printf("rio_readlineb = %s", buf);
  }
  return;
}

int make_request_to_server(char* url, char* host, char* port, char* method, char* version, char* filename) {
  int ptsfd; //proxy to server fd
  char *p;
  char buf[MAXLINE];
  rio_t pts_rio;

  ptsfd = Open_clientfd(host, port);
  Rio_readinitb(&pts_rio, ptsfd);

  sprintf(buf, "%s %s %s\r\n", method, url, version);
  sprintf(buf, "%sHost: %s:%s\r\n", buf, host, port);
  sprintf(buf, "%s%s", buf, user_agent_hdr);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);
  Rio_writen(ptsfd, buf, strlen(buf));
  Close(ptsfd);
}





// #include <stdio.h>
// #include "csapp.h"

// /* Recommended max cache and object sizes */
// #define MAX_CACHE_SIZE 1049000
// #define MAX_OBJECT_SIZE 102400

// /* You won't lose style points for including this long line in your code */
// static const char *user_agent_hdr =
//     "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
//     "Firefox/10.0.3\r\n";

// static const char *proxy_host;
// static const char *proxy_post;

// int main(int argc, char **argv)
// {
//   int listenfd, connfd; // 듣기소켓, 연결소켓 받아오기

//   // client의 hostname, port 임
//   char hostname[MAXLINE], port[MAXLINE];
//   socklen_t clientlen;
//   struct sockaddr_storage clientaddr;

//   if (argc != 2) // 에러검출
//   {

//     fprintf(stderr, "usage: %s <port>\n", argv[0]);
//     exit(1);
//   }

//   listenfd = Open_listenfd(argv[1]);
//   proxy_host = argv[0];
//   proxy_post = argv[1];
//   while (1)
//   {

//     clientlen = sizeof(clientaddr);

//     // Accept 실행 > 연결 받음 > 트랜잭션 수행 > 연결 닫음. (계속해서 반복)
//     // accept함수를 통해 연결 요청한 clientfd와 연결
//     connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

//     // Getnameinfo : ip 주소를 도메인주소로 바꿔줌
//     Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
//     printf("Accepted connection from (%s, %s)\n", hostname, port);

//     // client에서 받은 요청을 처리하는 doit 함수 진행
//     //  한 개의 HTTP 트랜잭션을 처리
//     //  요청 받은 데이터를 클라이언트에게 보내줌
//     doit(connfd);

//     Close(connfd);
//   }
// }

// void doit(int fd)
// {
//   // 프록시는 get요청만 받으면 됨. 정적인지 동적인지 몰라도됨 서버입장에서 프록시는 클라이언트 임. is_static 필요X
//   struct stat sbuf; // (필요함? 일단 가져옴)
//   // sbuf 에 메타데이터 정보가 저장됨 (필요함)
//   //
//   char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], url[MAXLINE], version[MAXLINE], port[MAXLINE], host[MAXLINE];
//   // hostname 목적지 필요하다.

//   char filename[MAXLINE]; // 환경변수 안쓰니 cgiargs 필요없음
//   rio_t client_rio;
//   char *p;

//   /* Read request line and headers*/
//   Rio_readinitb(&client_rio, fd);
//   Rio_readlineb(&client_rio, buf, MAXLINE);
//   printf("Request headers:\n");
//   printf("%s\n", buf); // \n 안들어가면 종종 출력이 안되거나 보기 안좋음, 이 buf에는 클라이언트의 요청이 저장됨.

//   // 메유버를 buf에 채워줌
//   sscanf(buf, "%s %s %s", method, uri, version);
//   // port가 있을경우, 없을경우도 고려
//   // 만약 입력에 http://가 포함되면 어떻게됨?

//   strcpy(url, uri);
//   if ((p = strchr(uri, '/')))
//   {
//     *p = '\0';
//     sscanf(p + 2, "%s", host);
//   }
//   else
//   {
//     strcpy(host, uri);
//   }

//   if ((p = strchr(host, ':')))
//   {
//     *p = '\0';                 // port 나누기
//     sscanf(host, "%s", host);  // port에 포트번호 담김
//     sscanf(p + 1, "%s", port); // uri에 ip주소만 담김
//     // uri = strtok(str,"")
//   }
//   if ((p = strchr(port, '/')))
//   {
//     *p = '\0';
//     sscanf(port, "%s", host);
//     sscanf(p + 1, "%s", filename);
//   }
//   if (strcasecmp(method, "GET"))
//   {
//     // clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
//     sprintf(buf, "GET요청이 아닙니다.\r\n");

//     Rio_writen(fd, buf, strlen(buf));
//     return;
//   }
//   read_requesthdrs(&client_rio);
// }

// void read_requesthdrs(rio_t *rp)
// // tiny 웹서버는 요청 헤더 내의 어떤 정보도 사용하지 않음
// // 따라서 요청 헤더를 종료하는 빈 텍스트줄(\r\n)이 나올 때까지 요청 헤더를 모두 읽어들임
// {
//   char buf[MAXLINE];

//   Rio_readlineb(rp, buf, MAXLINE);
//   printf("Rio_readline = %s", buf);

//   while (strcmp(buf, "\r\n"))
//   {
//     Rio_readlineb(rp, buf, MAXLINE);
//     printf("%s", buf);
//     // buf 가 비워짐(?) 다음 줄을 읽으면 2buf가 1buf를 덮어씀
//   }
//   printf("빠져나왔음\n");
//   return;
// }

// int make_request_to_server(url, host, port, method, version, filename) //
// {
//   int ptsfd; // proxy to server
//   char *p;
//   char buf[MAXLINE];
//   rio_t pts_rio;

//   ptsfd = Open_clientfd(host, port);
//   Rio_readinitb(&pts_rio, ptsfd);

//   sprintf(buf, "%s %s %s\r\n", method, url, version);
//   sprintf(buf, "%sHost: %s:%s\r\n", buf, host, port);
//   sprintf(buf, "%s%s", buf, user_agent_hdr);
//   sprintf(buf, "%sConnection: close\r\n", buf);
//   sprintf(buf, "%sProxy-Connection: close\r\n\r\n", buf);
//   Rio_writen(ptsfd, buf, strlen(buf));
//   Close(ptsfd);
// }
