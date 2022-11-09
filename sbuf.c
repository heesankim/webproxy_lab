//
// Created by 홍창섭 on 2022/11/09.
//

#include "csapp.h"
#include "sbuf.h"

/*Create an empty, bounded, shared FIFO buffer with N slots*/
void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    Sem_init(&sp->mutex, 0, 1);/* Binary semaphore for locking*/
    Sem_init(&sp->slots, 0, n);/*Initially, buf has n empty slots*/
    Sem_init(&sp->items, 0, 0);
}

/* Clean up Buffer sp*/
void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);
    P(&sp->mutex);
    sp->buf[(++sp->rear) % sp->n] = item;/* Insert the item */
    V(&sp->mutex);
    V(&sp->items);
}

int sbuf_remove(sbuf_t *sp){
    int item;
    P(&sp->items);
    P(&sp->mutex);
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);
    V(&sp->slots);
    return item;
}