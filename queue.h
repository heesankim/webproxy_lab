//큐 - 연결리스트 이용
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct Node //노드 정의
{
    // int data;
    struct Node *next;
    char *request_line;
    char *response;

} Node;

typedef struct Queue // Queue 구조체 정의
{
    Node *front; //맨 앞(꺼낼 위치)
    Node *rear;  //맨 뒤(보관할 위치)
    int count;   //보관 개수
} Queue;

void InitQueue(Queue *queue);                                 //큐 초기화
int IsEmpty(Queue *queue);                                    //큐가 비었는지 확인
void Enqueue(Queue *queue, char request_line, char response); //큐에 보관
void Dequeue(Queue *queue);                                   //큐에서 꺼냄