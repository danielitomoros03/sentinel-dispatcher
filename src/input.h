/*
 * input.h - Carga y validación del archivo de escenario.
 *
 * Formato de cada línea (las que empiezan con '#' y las vacías se ignoran):
 *     <llegada> <id> <cola> <ráfaga>
 *
 * El resultado es un arreglo de llegadas ordenado por tiempo de llegada y,
 * en caso de empate, por orden de aparición en el archivo. Ese arreglo es
 * el "guion" que el hilo maestro va consultando tic a tic.
 */
#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

/* Una llegada programada, tal como se leyó del archivo. */
typedef struct {
    long arrivalTime; /* tic de llegada (>= 0)                    */
    int  id;          /* identificador único (> 0)                */
    int  priority;    /* cola 0, 1 o 2                            */
    long burst;       /* ráfaga total (> 0)                       */
    long line;        /* número de línea en el archivo (desempate) */
} arrival_t;

/* Escenario completo: arreglo de llegadas ya ordenado. */
typedef struct {
    arrival_t *items;
    size_t     count;
} scenario_t;

/*
 * Lee el archivo path y llena sc. Si el archivo no existe o alguna línea es
 * inválida, imprime un mensaje claro en stderr y devuelve -1. Devuelve 0 si
 * todo salió bien. En caso de éxito, el llamador debe liberar con
 * scenario_free.
 */
int scenario_load(const char *path, scenario_t *sc);

/* Libera la memoria del escenario. */
void scenario_free(scenario_t *sc);

#endif /* INPUT_H */
