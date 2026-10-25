/*
 * sim.c - Hilo maestro, hilo trabajador y barrera de fase síncrona.
 *
 * Cada tic Tg se divide en dos fases separadas por una barrera de 2 hilos:
 *
 *        hilo maestro                     hilo trabajador (CPU_0)
 *   ------------------------         --------------------------------
 *   fase 1: admisión de Tg
 *   ---------------- barrera A: la admisión de Tg terminó ----------------
 *                                     fases 2, 3 y 4 y ejecución de 1 unidad
 *   ---------------- barrera B: el cómputo de Tg terminó -----------------
 *   ¿terminaron todos? -> fin
 *   si no, avanza el reloj a Tg+1
 *   (o salta hasta la próxima llegada si la CPU y las colas están vacías)
 *
 * La barrera A garantiza que la CPU nunca evalúa un tic antes de que estén
 * encoladas todas sus llegadas. La barrera B garantiza que el maestro no
 * cambia el reloj ni admite el tic siguiente mientras la CPU sigue
 * trabajando. Así el orden de las fases es siempre el mismo y la salida es
 * determinista, aunque haya dos hilos.
 *
 * Además, el mutex del planificador protege las colas y el estado en ambas
 * fases, de modo que cada inserción y cada decisión de la CPU es atómica.
 *
 * Para terminar, el maestro marca done y pasa una vez más por la barrera A.
 * El trabajador, que estaba esperando ahí, ve done y sale de su ciclo.
 */
#include <stdio.h>

#include "log.h"
#include "sim.h"

/* Datos compartidos por los dos hilos. */
typedef struct {
    scheduler_t      *s;
    const scenario_t *sc;
    pthread_barrier_t barrier;
    int               done;   /* 1 cuando la simulación terminó   */
    int               error;  /* 1 si el maestro no pudo crear un TCB */
} sim_t;

/*
 * Hilo maestro: controla el reloj y admite las llegadas de cada tic.
 */
static void *master_thread(void *arg)
{
    sim_t       *sim  = arg;
    scheduler_t *s    = sim->s;
    size_t       next = 0;   /* próxima llegada del escenario (ya ordenado) */

    for (;;) {
        /* Fase 1: admisión de todas las llegadas programadas para Tg. */
        pthread_mutex_lock(&s->lock);
        log_tick(s->clock);
        while (next < sim->sc->count
               && sim->sc->items[next].arrivalTime == s->clock) {
            if (scheduler_admit(s, &sim->sc->items[next]) != 0) {
                sim->error = 1;
                break;
            }
            next++;
        }
        if (sim->error)
            sim->done = 1;
        pthread_mutex_unlock(&s->lock);

        pthread_barrier_wait(&sim->barrier);   /* A: admisión lista */
        if (sim->done)
            break;
        pthread_barrier_wait(&sim->barrier);   /* B: cómputo listo  */

        /* Decidir si se termina o a qué tic se avanza. */
        pthread_mutex_lock(&s->lock);
        if (scheduler_all_finished(s)) {
            sim->done = 1;
        } else if (scheduler_is_idle(s) && next < sim->sc->count
                   && sim->sc->items[next].arrivalTime > s->clock + 1) {
            /* Tics ociosos: nadie listo ni en CPU. El reloj salta hasta la
             * próxima llegada en vez de recorrer los tics vacíos uno a uno. */
            log_trace("no sentinel until T-%04ld: clock jumps from T-%04ld "
                      "(CPU_0 idle %ld ticks)",
                      sim->sc->items[next].arrivalTime, s->clock + 1,
                      sim->sc->items[next].arrivalTime - (s->clock + 1));
            s->clock = sim->sc->items[next].arrivalTime;
        } else {
            s->clock++;
        }
        pthread_mutex_unlock(&s->lock);

        if (sim->done) {
            /* Libera al trabajador, que espera en la barrera A. */
            pthread_barrier_wait(&sim->barrier);
            break;
        }
    }
    return NULL;
}

/*
 * Hilo trabajador (CPU_0): en cada tic, después de la admisión, aplica las
 * fases 2 a 4 y ejecuta una unidad de ráfaga.
 */
static void *worker_thread(void *arg)
{
    sim_t       *sim = arg;
    scheduler_t *s   = sim->s;

    for (;;) {
        pthread_barrier_wait(&sim->barrier);   /* A: espera la admisión */
        if (sim->done)
            break;

        pthread_mutex_lock(&s->lock);
        scheduler_cpu_tick(s);
        pthread_mutex_unlock(&s->lock);

        pthread_barrier_wait(&sim->barrier);   /* B: avisa que terminó  */
    }
    return NULL;
}

int sim_run(scheduler_t *s, const scenario_t *sc)
{
    sim_t     sim;
    pthread_t master, worker;

    sim.s     = s;
    sim.sc    = sc;
    sim.done  = 0;
    sim.error = 0;

    if (pthread_barrier_init(&sim.barrier, NULL, 2) != 0) {
        fprintf(stderr, "Error: no se pudo crear la barrera\n");
        return -1;
    }

    if (pthread_create(&worker, NULL, worker_thread, &sim) != 0) {
        fprintf(stderr, "Error: no se pudo crear el hilo trabajador\n");
        pthread_barrier_destroy(&sim.barrier);
        return -1;
    }
    if (pthread_create(&master, NULL, master_thread, &sim) != 0) {
        fprintf(stderr, "Error: no se pudo crear el hilo maestro\n");
        /* El trabajador ya espera en la barrera A: el hilo principal ocupa
         * el lugar del maestro para liberarlo con done = 1. */
        sim.done = 1;
        pthread_barrier_wait(&sim.barrier);
        pthread_join(worker, NULL);
        pthread_barrier_destroy(&sim.barrier);
        return -1;
    }

    pthread_join(master, NULL);
    pthread_join(worker, NULL);
    pthread_barrier_destroy(&sim.barrier);
    return sim.error ? -1 : 0;
}
