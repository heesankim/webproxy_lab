//큐 - 연결리스트 이용
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct Node //노드 정의
{
    // int data;
    struct Node *next;
    char request_line[MAXLINE];
    char response[MAXLINE];

} Node;

typedef struct Queue // Queue 구조체 정의
{
    Node *front; //맨 앞(꺼낼 위치)
    Node *rear;  //맨 뒤(보관할 위치)
    int count;   //보관 개수
} Queue;

void InitQueue(Queue *queue);                                   //큐 초기화
int IsEmpty(Queue *queue);                                      //큐가 비었는지 확인
void Enqueue(Queue *queue, char *request_line, char *response); //큐에 보관
void Dequeue(Queue *queue);                                     //큐에서 꺼냄

void InitQueue(Queue *queue)
{
    queue->front = queue->rear = NULL; // front와 rear를 NULL로 설정
    queue->count = 0;                  //보관 개수를 0으로 설정
}

int IsEmpty(Queue *queue)
{
    return queue->count == 0; //보관 개수가 0이면 빈 상태
}

void Enqueue(Queue *queue, char *request_line, char *response)
{
    Node *now = (Node *)calloc(1, MAX_OBJECT_SIZE); //노드 생성
    strcpy(now->request_line, request_line);        //데이터 설정
    strcpy(now->response, response);
    now->next = NULL;
    if (IsEmpty(queue)) //큐가 비어있을 때
    {
        queue->front = now; //맨 앞을 now로 설정
    }
    else //비어있지 않을 때
    {
        queue->rear->next = now; //맨 뒤의 다음을 now로 설정
    }
    queue->rear = now; //맨 뒤를 now로 설정
    queue->count++;    //보관 개수를 1 증가
}

void Dequeue(Queue *queue)
{
    int re = 0;
    Node *now;
    if (IsEmpty(queue)) //큐가 비었을 때
    {
        return re;
    }
    now = queue->front; //맨 앞의 노드를 now에 기억
    // re = now->data;//반환할 값은 now의 data로 설정
    queue->front = now->next; //맨 앞은 now의 다음 노드로 설정
    free(now);                // now 소멸
    queue->count--;           //보관 개수를 1 감소
    return;
}