/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "../csapp.h"

int main(void) {

  char *buf, *p, *p1, *p2;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1=0, n2=0;

  /* Extract the two arguments */
  if((buf = getenv("QUERY_STRING")) != NULL)
  { //buf -> n1=123&n2=123
    p = strchr(buf, '&'); // ? 뒤부터 buf 이 buf에서 &의 위치(주소값)를 반환
    
    // * 역참조
    *p = '\0';  // buf -> n1=123\0n2=123
    strcpy(arg1, buf); // 뒤에 있는 buf 문자열을 arg1에 덮어써라(null 문자 만날때까지). -> arg1 -> n1=123\0
    // strcpy 쓸때 null만나면 null포함하고 스톱함 ,탈출조건이다.
    p1 = strchr(arg1, '='); // p1 =위치 가리킴
    strcpy(arg1,p1+1);// arg1 -> 123\0 arg1 complete
    strcpy(arg2, p+1); // 인자로 포인터(위치)가 들어올 수 있다. arg2 -> n2=123
    p2 = strchr(arg2, '='); // p2 위치 설정
    strcpy(arg2, p2+1);
    n1 = atoi(arg1); // atoi는 문자열을 정수형으로 바꿔줌.
    n2 = atoi(arg2);
  }

  
  /* Make the response body */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com: ");
  sprintf(content, "%sTHE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%sThe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1+n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  /* Generate the HTTP response */
  printf("Connection: close\r\n");
  printf("Content-length: %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */