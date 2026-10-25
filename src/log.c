/*
 * log.c - Implementación de la salida estándar y verbose.
 */
#include <stdarg.h>
#include <stdio.h>

#include "log.h"

/* Indica si se pidió -v / --verbose. Se fija una sola vez en main. */
static int verbose = 0;

/* Prefijo de las trazas verbose: alinea el texto después de "[T-0000] ". */
#define TRACE_PREFIX "         | "

void log_set_verbose(int on)
{
    verbose = on;
}

int log_is_verbose(void)
{
    return verbose;
}

void log_header(void)
{
    printf("Fixed parameters: Queue 0 [Q=%ld] | Queue 1 [Q=%ld] | Queue 2 [FCFS]\n\n",
           queue_quantum(0), queue_quantum(1));
}

void log_arrival(long tick, const tcb_t *t)
{
    printf("[T-%04ld] ARRIVAL: Sentinel ID:%d [Type: %s]\n",
           tick, t->id, queue_type_name(t->priority));
}

void log_quantum_expired(long tick, const tcb_t *t)
{
    printf("[T-%04ld] QUANTUM_EXPIRED: Sentinel %d yields CPU via RR rotation (Q=%ld)\n",
           tick, t->id, queue_quantum(t->priority));
}

void log_preempted(long tick, const tcb_t *t, int byQueue)
{
    printf("[T-%04ld] PREEMPTED: Sentinel %d preempted by higher-priority queue %d\n",
           tick, t->id, byQueue);
}

void log_completed(long tick, const tcb_t *t)
{
    printf("[T-%04ld] COMPLETED: Sentinel ID:%d -> Turnaround Time: %ld ticks\n",
           tick, t->id, t->returnTime);
}

void log_tick(long tick)
{
    if (!verbose)
        return;
    printf("---------------------------- T-%04ld ----------------------------\n", tick);
}

void log_trace(const char *fmt, ...)
{
    va_list ap;

    if (!verbose)
        return;
    fputs(TRACE_PREFIX, stdout);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
}

void log_queues(const queue_t queues[NUM_QUEUES])
{
    int i;

    if (!verbose)
        return;
    for (i = 0; i < NUM_QUEUES; i++) {
        const tcb_t *t;
        long q = queue_quantum(i);

        /* Encabezado de la cola con su tipo y algoritmo. */
        if (q > 0)
            printf(TRACE_PREFIX "Queue %d [%-7s RR Q=%ld]:", i, queue_type_name(i), q);
        else
            printf(TRACE_PREFIX "Queue %d [%-7s FCFS ]:", i, queue_type_name(i));

        if (queue_is_empty(&queues[i]))
            fputs(" (empty)", stdout);

        /* Cada centinela con su ráfaga restante y, en RR, su quantum usado.
         * Los expropiados se marcan con '*'. */
        for (t = queues[i].head; t != NULL; t = t->next) {
            const char *mark = t->state == STATE_INTERRUPTED ? "*" : "";
            if (q > 0)
                printf(" %d%s(rem %ld, q %ld/%ld)", t->id, mark,
                       t->remainingBurst, t->usedQuantum, q);
            else
                printf(" %d%s(rem %ld)", t->id, mark, t->remainingBurst);
        }
        putchar('\n');
    }
}

void log_summary(tcb_t *const *all, size_t n, long contextSwitches)
{
    size_t i;

    if (!verbose)
        return;
    printf("\nPer-sentinel summary:\n");
    printf("  %8s  %-7s  %7s  %7s  %7s  %10s  %7s\n",
           "ID", "Type", "Arrival", "Burst", "Finish", "Turnaround", "Waiting");
    for (i = 0; i < n; i++) {
        const tcb_t *t = all[i];
        printf("  %8d  %-7s  %7ld  %7ld  %7ld  %10ld  %7ld\n",
               t->id, queue_type_name(t->priority), t->arrivalTime,
               t->totalBurst, t->finishTime, t->returnTime, t->waitTime);
    }
    printf("  Context switches: %ld\n", contextSwitches);
}

void log_metrics(long totalTime, double throughput, double avgWait)
{
    /* Formato copiado tal cual de la salida esperada del enunciado,
     * incluida la diferencia de espacios antes de los dos puntos. */
    printf("\n==================== GLOBAL PERFORMANCE METRICS ====================\n");
    printf("        - Total Simulation Time : %ld clock ticks.\n", totalTime);
    printf("        - System Throughput     : %.2f sentinels/tick.\n", throughput);
    printf("        - Average Waiting Time : %.2f ticks.\n", avgWait);
    printf("====================================================================\n");
}
