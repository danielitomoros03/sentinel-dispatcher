/*
 * queue.h - Cola FIFO de TCBs implementada como lista enlazada.
 *
 * Cada cola de prioridad del planificador es una de estas. Se necesitan tres
 * operaciones:
 *   - push_back:  encolar al final (llegadas y rotación por quantum).
 *   - push_front: devolver a la cabeza (centinela expropiado).
 *   - pop_front:  sacar la cabeza (despacho).
 * Todas son O(1) porque se guardan punteros a la cabeza y a la cola.
 *
 * La cola no reserva memoria: enlaza los TCB a través de su campo next.
 * La sincronización entre hilos la hace el llamador (mutex del planificador).
 */
#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>

#include "tcb.h"

typedef struct {
    tcb_t  *head;  /* primer TCB (el próximo a despachar) */
    tcb_t  *tail;  /* último TCB                          */
    size_t  size;  /* cantidad de TCB en la cola          */
} queue_t;

/* Deja la cola vacía. */
void queue_init(queue_t *q);

/* Agrega t al final de la cola. */
void queue_push_back(queue_t *q, tcb_t *t);

/* Agrega t en la cabeza de la cola. */
void queue_push_front(queue_t *q, tcb_t *t);

/* Saca y devuelve la cabeza de la cola, o NULL si está vacía. */
tcb_t *queue_pop_front(queue_t *q);

/* Devuelve 1 si la cola está vacía y 0 si no. */
int queue_is_empty(const queue_t *q);

#endif /* QUEUE_H */
