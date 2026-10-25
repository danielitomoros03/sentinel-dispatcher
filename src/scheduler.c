/*
 * scheduler.c - Lógica del planificador: fases del tic, despacho y métricas.
 */
#include <stdio.h>
#include <stdlib.h>

#include "log.h"
#include "scheduler.h"

int scheduler_init(scheduler_t *s, size_t total)
{
    int i;

    for (i = 0; i < NUM_QUEUES; i++)
        queue_init(&s->queues[i]);
    s->running         = NULL;
    s->clock           = 0;
    s->total           = total;
    s->created         = 0;
    s->finished        = 0;
    s->lastFinish      = 0;
    s->contextSwitches = 0;

    /* Arreglo con un puntero por centinela; se llena al admitirlos. */
    s->all = calloc(total, sizeof *s->all);
    if (s->all == NULL) {
        fprintf(stderr, "Error: memoria insuficiente\n");
        return -1;
    }
    if (pthread_mutex_init(&s->lock, NULL) != 0) {
        fprintf(stderr, "Error: no se pudo crear el mutex\n");
        free(s->all);
        return -1;
    }
    return 0;
}

void scheduler_destroy(scheduler_t *s)
{
    size_t i;

    /* Todos los TCB están en all, sin importar en qué cola quedaron. */
    for (i = 0; i < s->created; i++)
        tcb_destroy(s->all[i]);
    free(s->all);
    s->all = NULL;
    pthread_mutex_destroy(&s->lock);
}

/*
 * Devuelve el índice de la cola no vacía de mayor prioridad entre las colas
 * 0 .. limit-1, o -1 si todas están vacías. Con limit = NUM_QUEUES revisa
 * todas; con limit = p revisa solo las estrictamente superiores a p.
 */
static int highest_ready(const scheduler_t *s, int limit)
{
    int i;

    for (i = 0; i < limit; i++) {
        if (!queue_is_empty(&s->queues[i]))
            return i;
    }
    return -1;
}

int scheduler_admit(scheduler_t *s, const arrival_t *a)
{
    tcb_t *t = tcb_create(a->id, a->priority, a->arrivalTime, a->burst);

    if (t == NULL) {
        fprintf(stderr, "Error: memoria insuficiente al crear el TCB %d\n", a->id);
        return -1;
    }
    s->all[s->created++] = t;

    /* NEW -> READY: entra al final de su cola. */
    t->state = STATE_READY;
    queue_push_back(&s->queues[t->priority], t);
    log_arrival(s->clock, t);
    return 0;
}

/*
 * Marca como terminado al centinela t en el tic actual y calcula sus
 * tiempos. La ráfaga se completó durante el tic anterior, así que el
 * instante de finalización es el tic actual (Tg + 1 del último tic ejecutado).
 */
static void finish(scheduler_t *s, tcb_t *t)
{
    t->state      = STATE_FINISHED;
    t->finishTime = s->clock;
    t->returnTime = t->finishTime - t->arrivalTime;   /* turnaround */
    t->waitTime   = t->returnTime - t->totalBurst;    /* espera     */
    s->finished++;
    s->lastFinish = s->clock;
    s->running    = NULL;
    log_completed(s->clock, t);
}

/*
 * Fase 2: retiro y expiración de quantum.
 *
 * Se revisa el centinela que ejecutó en el tic anterior:
 *   - Si ya no le queda ráfaga, termina. La terminación tiene prioridad: si
 *     también agotó su quantum justo en ese tic, no hay QUANTUM_EXPIRED.
 *   - Si es RR y agotó su quantum, se reencola al final de su cola. Como la
 *     admisión ya ocurrió, queda detrás de los que llegaron en este tic.
 *
 * QUANTUM_EXPIRED solo se imprime si otro centinela va a tomar la CPU (hay
 * un cambio de contexto real). Si el que expiró es el único listo, el
 * despacho lo vuelve a elegir con un quantum nuevo y no se imprime nada en
 * modo estándar; así lo muestra la salida esperada con el centinela 201 en
 * T9 del caso de referencia.
 */
static void phase_retire_or_rotate(scheduler_t *s)
{
    tcb_t *t = s->running;
    long q;
    int  others;

    if (t == NULL)
        return;

    if (t->remainingBurst == 0) {
        finish(s, t);
        return;
    }

    q = queue_quantum(t->priority);
    if (q == 0 || t->usedQuantum < q)
        return;   /* FCFS, o todavía le queda quantum */

    /* RUNNING -> READY: al final de su cola, con el quantum en cero. */
    s->running = NULL;
    log_trace("rotation: %d used its quantum (%ld/%ld) with %ld left; "
              "re-queued at tail of Queue %d",
              t->id, t->usedQuantum, q, t->remainingBurst, t->priority);
    t->usedQuantum = 0;
    t->state = STATE_READY;
    queue_push_back(&s->queues[t->priority], t);

    /* El despacho elegirá a otro si hay alguien en una cola superior o si
     * en su propia cola hay alguien además de él. */
    others = highest_ready(s, t->priority) >= 0
             || s->queues[t->priority].size > 1;
    if (others)
        log_quantum_expired(s->clock, t);
    else
        log_trace("no other sentinel is ready: %d gets a new quantum "
                  "(no context switch, QUANTUM_EXPIRED not reported)", t->id);
}

/*
 * Fase 3: expropiación.
 *
 * Si el centinela activo sigue en la CPU (no terminó ni expiró en la fase 2)
 * y hay alguien listo en una cola estrictamente superior, se le quita la CPU
 * y vuelve a la cabeza de su cola. Conserva usedQuantum y remainingBurst, así
 * que al reanudarse sigue con el quantum que le quedaba. En la cola 2 (FCFS)
 * esto es lo único que puede sacarlo de la CPU antes de terminar.
 */
static void phase_preempt(scheduler_t *s)
{
    tcb_t *t = s->running;
    int    hq;

    if (t == NULL)
        return;

    hq = highest_ready(s, t->priority);
    if (hq < 0)
        return;

    /* RUNNING -> INTERRUPTED: a la cabeza de su cola. */
    s->running = NULL;
    t->state = STATE_INTERRUPTED;
    queue_push_front(&s->queues[t->priority], t);
    log_preempted(s->clock, t, hq);
    if (queue_quantum(t->priority) > 0)
        log_trace("preemption: %d back to head of Queue %d, keeps quantum "
                  "%ld/%ld and %ld burst left", t->id, t->priority,
                  t->usedQuantum, queue_quantum(t->priority), t->remainingBurst);
    else
        log_trace("preemption: %d back to head of Queue %d, keeps %ld burst left",
                  t->id, t->priority, t->remainingBurst);
}

/*
 * Fase 4: despacho.
 *
 * Si la CPU quedó libre, toma la cabeza de la cola no vacía de mayor
 * prioridad. Si el activo sigue en la CPU, continúa sin cambios.
 */
static void phase_dispatch(scheduler_t *s)
{
    tcb_t *t;
    int    hq;

    log_queues(s->queues);

    if (s->running != NULL) {
        log_trace("dispatch: %d keeps CPU_0", s->running->id);
        return;
    }

    hq = highest_ready(s, NUM_QUEUES);
    if (hq < 0) {
        log_trace("dispatch: all queues empty");
        return;
    }

    /* READY/INTERRUPTED -> RUNNING. */
    t = queue_pop_front(&s->queues[hq]);
    t->state = STATE_RUNNING;
    s->running = t;
    log_trace("dispatch: %d taken from head of Queue %d", t->id, hq);
}

/*
 * Ejecución: CPU_0 procesa una unidad de ráfaga del centinela activo. El
 * resultado (terminó o agotó quantum) se evalúa en la fase 2 del tic
 * siguiente, que es cuando ese instante se observa.
 */
static void execute(scheduler_t *s)
{
    tcb_t *t = s->running;
    long   q;

    if (t == NULL) {
        if (scheduler_all_finished(s))
            log_trace("all %zu sentinels finished: simulation ends", s->total);
        else
            log_trace("CPU_0 IDLE");
        return;
    }

    t->remainingBurst--;
    t->usedQuantum++;

    q = queue_quantum(t->priority);
    if (q > 0)
        log_trace("CPU_0 runs %d: quantum %ld/%ld, burst left %ld",
                  t->id, t->usedQuantum, q, t->remainingBurst);
    else
        log_trace("CPU_0 runs %d (FCFS): burst left %ld",
                  t->id, t->remainingBurst);
}

void scheduler_cpu_tick(scheduler_t *s)
{
    /* Quién tenía la CPU al empezar el tic, para detectar el cambio de contexto. */
    tcb_t *prev = s->running;

    phase_retire_or_rotate(s);
    phase_preempt(s);
    phase_dispatch(s);

    /* Hay cambio de contexto cuando la CPU pasa a otro centinela. Si el que
     * expiró era el único listo y se vuelve a despachar, no lo hay. */
    if (s->running != NULL && s->running != prev) {
        s->contextSwitches++;
        if (prev == NULL)
            log_trace("context switch: (idle) -> %d", s->running->id);
        else if (prev->state == STATE_FINISHED)
            log_trace("context switch: %d (finished) -> %d", prev->id, s->running->id);
        else
            log_trace("context switch: %d -> %d", prev->id, s->running->id);
    }

    execute(s);
}

int scheduler_is_idle(const scheduler_t *s)
{
    return s->running == NULL && highest_ready(s, NUM_QUEUES) < 0;
}

int scheduler_all_finished(const scheduler_t *s)
{
    return s->finished == s->total;
}

void scheduler_report(const scheduler_t *s)
{
    size_t i;
    long   sumWait = 0;
    double throughput, avgWait;

    /* Espera de cada centinela = turnaround - ráfaga (calculada en finish). */
    for (i = 0; i < s->created; i++)
        sumWait += s->all[i]->waitTime;

    /* El tiempo total es el tic de la última finalización. Nunca es 0,
     * porque toda ráfaga es >= 1 y termina como mínimo en el tic 1. */
    throughput = (double)s->total / (double)s->lastFinish;
    avgWait    = (double)sumWait / (double)s->total;

    log_summary(s->all, s->created, s->contextSwitches);
    log_metrics(s->lastFinish, throughput, avgWait);
}
