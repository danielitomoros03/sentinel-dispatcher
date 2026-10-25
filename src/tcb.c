/*
 * tcb.c - Creación del TCB y tablas de nombres y quantums por cola.
 */
#include <stdlib.h>

#include "tcb.h"

/* Quantum de cada cola, indexado por prioridad. 0 = FCFS (sin quantum). */
static const long QUANTUMS[NUM_QUEUES] = { 2, 4, 0 };

/* Nombre de cada tipo de cola, indexado por prioridad. */
static const char *const TYPE_NAMES[NUM_QUEUES] = { "Command", "Driller", "Heavy" };

tcb_t *tcb_create(int id, int priority, long arrivalTime, long totalBurst)
{
    tcb_t *t = malloc(sizeof *t);
    if (t == NULL)
        return NULL;

    /* Al nacer, al centinela le falta toda su ráfaga y no ha usado quantum.
     * Los tiempos de finalización, retorno y espera se fijan al terminar. */
    t->id             = id;
    t->state          = STATE_NEW;
    t->priority       = priority;
    t->arrivalTime    = arrivalTime;
    t->totalBurst     = totalBurst;
    t->remainingBurst = totalBurst;
    t->usedQuantum    = 0;
    t->finishTime     = -1;
    t->returnTime     = -1;
    t->waitTime       = -1;
    t->next           = NULL;
    return t;
}

void tcb_destroy(tcb_t *t)
{
    free(t);
}

const char *tcb_state_name(tcb_state_t s)
{
    switch (s) {
    case STATE_NEW:         return "NEW";
    case STATE_READY:       return "READY";
    case STATE_RUNNING:     return "RUNNING";
    case STATE_INTERRUPTED: return "INTERRUPTED";
    case STATE_FINISHED:    return "FINISHED";
    }
    return "?";
}

const char *queue_type_name(int priority)
{
    return TYPE_NAMES[priority];
}

long queue_quantum(int priority)
{
    return QUANTUMS[priority];
}
