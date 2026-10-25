/*
 * queue.c - Operaciones de la cola FIFO enlazada.
 */
#include "queue.h"

void queue_init(queue_t *q)
{
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

void queue_push_back(queue_t *q, tcb_t *t)
{
    t->next = NULL;
    if (q->tail == NULL) {
        /* Cola vacía: t pasa a ser cabeza y cola a la vez. */
        q->head = t;
    } else {
        q->tail->next = t;
    }
    q->tail = t;
    q->size++;
}

void queue_push_front(queue_t *q, tcb_t *t)
{
    t->next = q->head;
    q->head = t;
    if (q->tail == NULL) {
        /* Cola vacía: t también es el último. */
        q->tail = t;
    }
    q->size++;
}

tcb_t *queue_pop_front(queue_t *q)
{
    tcb_t *t = q->head;
    if (t == NULL)
        return NULL;

    q->head = t->next;
    if (q->head == NULL) {
        /* Se sacó el único elemento: la cola queda vacía. */
        q->tail = NULL;
    }
    t->next = NULL;
    q->size--;
    return t;
}

int queue_is_empty(const queue_t *q)
{
    return q->head == NULL;
}
