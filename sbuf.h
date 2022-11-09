//
// Created by 홍창섭 on 2022/11/09.
//

#ifndef PROXY_WEB_SERVER_SUBF_F_H
#define PROXY_WEB_SERVER_SUBF_F_H

/*
 * slot : socket 연결된 fd 값이 들어가는 공간
 * */
typedef struct{
    int* buf;       /*Buffer Array*/
    int n;          /* Maximun number of slots */
    int front;      /* buf[(front + 1) % n] is first item 가장 먼저 slot에서 빠져 나올 fd값을 보관하는 Index*/
    int rear;       /* buf[rear % n] is last item 새로운 fd값을 넣을 자리*/
    sem_t mutex;
    sem_t slots;
    sem_t items;
}sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

#endif //PROXY_WEB_SERVER_SUBF_F_H
