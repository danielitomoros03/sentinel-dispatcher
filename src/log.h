/*
 * log.h - Salida del simulador en modo estándar y en modo verbose.
 *
 * Todo lo que el programa imprime pasa por este módulo, así el formato
 * exacto vive en un solo lugar.
 *
 *   - Modo estándar: la línea "Fixed parameters...", los eventos ARRIVAL,
 *     QUANTUM_EXPIRED, PREEMPTED y COMPLETED, y el bloque de métricas.
 *   - Modo verbose (-v): lo mismo, más trazas por tic con el estado de las
 *     colas, la decisión del despachador, el quantum consumido y los
 *     cambios de contexto. Las trazas empiezan con "         | " para que
 *     se distingan de los eventos.
 *
 * Las funciones no se protegen con mutex propio: los hilos solo las llaman
 * mientras tienen tomado el mutex del planificador.
 */
#ifndef LOG_H
#define LOG_H

#include <stddef.h>

#include "queue.h"
#include "tcb.h"

/* Activa (1) o desactiva (0) el modo verbose. */
void log_set_verbose(int on);

/* Devuelve 1 si el modo verbose está activo. */
int log_is_verbose(void);

/* Línea de parámetros fijos y la línea en blanco que la sigue. */
void log_header(void);

/* Eventos del modo estándar (se imprimen siempre). */
void log_arrival(long tick, const tcb_t *t);
void log_quantum_expired(long tick, const tcb_t *t);
void log_preempted(long tick, const tcb_t *t, int byQueue);
void log_completed(long tick, const tcb_t *t);

/* --- Trazas del modo verbose (no imprimen nada en modo estándar). --- */

/* Separador con el número de tic, al comienzo de cada tic. */
void log_tick(long tick);

/* Una línea de traza con formato printf. */
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
void log_trace(const char *fmt, ...);

/* Contenido de las tres colas de listos. */
void log_queues(const queue_t queues[NUM_QUEUES]);

/* Tabla por centinela y total de cambios de contexto, al final. */
void log_summary(tcb_t *const *all, size_t n, long contextSwitches);

/* Bloque final de métricas globales (se imprime siempre). */
void log_metrics(long totalTime, double throughput, double avgWait);

#endif /* LOG_H */
