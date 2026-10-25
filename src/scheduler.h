/*
 * scheduler.h - Estado del planificador multinivel y fases de cada tic.
 *
 * Orden de fases en cada tic Tg:
 *   1. Admisión      (hilo maestro): las llegadas de Tg van al final de su cola.
 *   2. Retiro y expiración (hilo trabajador): se revisa el centinela que
 *      ejecutó en el tic anterior. Si su ráfaga llegó a 0, termina en Tg
 *      (COMPLETED). Si agotó su quantum, va al final de su cola, detrás de
 *      los admitidos en la fase 1.
 *   3. Expropiación  (hilo trabajador): si el activo aún tiene quantum pero
 *      hay alguien listo en una cola estrictamente superior, vuelve a la
 *      cabeza de su cola y conserva usedQuantum y remainingBurst.
 *   4. Despacho      (hilo trabajador): si la CPU está libre, toma la cabeza
 *      de la cola no vacía de mayor prioridad.
 * Después la CPU ejecuta 1 unidad de ráfaga del centinela activo.
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pthread.h>
#include <stddef.h>

#include "input.h"
#include "queue.h"
#include "tcb.h"

typedef struct {
    queue_t   queues[NUM_QUEUES]; /* colas de listos 0, 1 y 2                 */
    tcb_t    *running;            /* centinela en CPU_0, o NULL si está ociosa */
    long      clock;              /* tic global actual (Tg)                    */

    tcb_t   **all;                /* todos los TCB creados, en orden de llegada */
    size_t    total;              /* número de centinelas del escenario (n)    */
    size_t    created;            /* TCB creados hasta ahora                   */
    size_t    finished;           /* centinelas que ya terminaron              */
    long      lastFinish;         /* tic de la última finalización             */
    long      contextSwitches;    /* veces que cambió el dueño de la CPU       */

    /* Protege todo lo anterior. Lo toman el hilo maestro (admisión) y el
     * hilo trabajador (fases 2 a 4 y ejecución). */
    pthread_mutex_t lock;
} scheduler_t;

/* Prepara un planificador vacío para total centinelas. 0 = éxito, -1 = error. */
int scheduler_init(scheduler_t *s, size_t total);

/* Libera los TCB y el mutex. */
void scheduler_destroy(scheduler_t *s);

/*
 * Fase 1 (admisión): crea el TCB de la llegada a y lo pone al final de su
 * cola. La llama el hilo maestro con el mutex tomado. 0 = éxito, -1 = sin memoria.
 */
int scheduler_admit(scheduler_t *s, const arrival_t *a);

/*
 * Fases 2, 3 y 4 y ejecución de una unidad de ráfaga. La llama el hilo
 * trabajador con el mutex tomado, después de la admisión del mismo tic.
 */
void scheduler_cpu_tick(scheduler_t *s);

/* 1 si la CPU está libre y las tres colas están vacías. */
int scheduler_is_idle(const scheduler_t *s);

/* 1 si ya terminaron todos los centinelas. */
int scheduler_all_finished(const scheduler_t *s);

/* Imprime el resumen verbose y el bloque de métricas globales. */
void scheduler_report(const scheduler_t *s);

#endif /* SCHEDULER_H */
