/*
 * sim.h - Hilos de la simulación: maestro (reloj e inserción) y trabajador (CPU_0).
 */
#ifndef SIM_H
#define SIM_H

#include "input.h"
#include "scheduler.h"

/*
 * Corre la simulación completa del escenario sc sobre el planificador s.
 * Crea el hilo maestro y el hilo trabajador, espera a que terminen y
 * devuelve 0 si todo salió bien o -1 si hubo un error.
 */
int sim_run(scheduler_t *s, const scenario_t *sc);

#endif /* SIM_H */
