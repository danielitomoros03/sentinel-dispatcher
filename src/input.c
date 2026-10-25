/*
 * input.c - Lectura, validación y ordenamiento del archivo de escenario.
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "tcb.h"

/*
 * Límite superior para tiempos de llegada y ráfagas. Es muy holgado y sirve
 * solo para que la suma de tiempos nunca desborde un long.
 */
#define MAX_TIME 1000000000L

/* Devuelve 1 si la línea solo tiene espacios o es un comentario ('#'). */
static int is_blank_or_comment(const char *line)
{
    while (isspace((unsigned char)*line))
        line++;
    return *line == '\0' || *line == '#';
}

/*
 * Lee un entero en base 10 desde *p y avanza *p hasta después del número.
 * Devuelve 0 si había un número válido y -1 si no (texto no numérico o
 * fuera de rango de long).
 */
static int read_long(const char **p, long *out)
{
    char *end;

    errno = 0;
    *out = strtol(*p, &end, 10);
    if (end == *p || errno == ERANGE)
        return -1;
    /* El número debe terminar en espacio o fin de línea: "3x" no es válido. */
    if (*end != '\0' && !isspace((unsigned char)*end))
        return -1;
    *p = end;
    return 0;
}

/*
 * Interpreta una línea de datos. Escribe un mensaje de error en stderr y
 * devuelve -1 si la línea es inválida.
 */
static int parse_line(const char *path, long lineno, const char *line,
                      arrival_t *a)
{
    long v[4];
    const char *p = line;
    int i;

    for (i = 0; i < 4; i++) {
        if (read_long(&p, &v[i]) != 0) {
            fprintf(stderr,
                    "Error: %s:%ld: se esperaban 4 enteros "
                    "(llegada id cola rafaga)\n", path, lineno);
            return -1;
        }
    }
    /* No debe quedar nada más que espacios después del cuarto número. */
    while (isspace((unsigned char)*p))
        p++;
    if (*p != '\0') {
        fprintf(stderr,
                "Error: %s:%ld: hay texto de mas despues de la rafaga\n",
                path, lineno);
        return -1;
    }

    if (v[0] < 0 || v[0] > MAX_TIME) {
        fprintf(stderr,
                "Error: %s:%ld: tiempo de llegada invalido (%ld); "
                "debe estar entre 0 y %ld\n", path, lineno, v[0], MAX_TIME);
        return -1;
    }
    if (v[1] <= 0 || v[1] > INT_MAX) {
        fprintf(stderr,
                "Error: %s:%ld: ID invalido (%ld); debe ser un entero "
                "positivo\n", path, lineno, v[1]);
        return -1;
    }
    if (v[2] < 0 || v[2] >= NUM_QUEUES) {
        fprintf(stderr,
                "Error: %s:%ld: cola invalida (%ld) para el ID %ld; "
                "debe ser 0, 1 o 2\n", path, lineno, v[2], v[1]);
        return -1;
    }
    if (v[3] <= 0 || v[3] > MAX_TIME) {
        fprintf(stderr,
                "Error: %s:%ld: rafaga invalida (%ld) para el ID %ld; "
                "debe ser mayor que 0\n", path, lineno, v[3], v[1]);
        return -1;
    }

    a->arrivalTime = v[0];
    a->id          = (int)v[1];
    a->priority    = (int)v[2];
    a->burst       = v[3];
    a->line        = lineno;
    return 0;
}

/*
 * Comparador para ordenar las llegadas. qsort no es estable, así que el
 * número de línea se usa como segundo criterio: dos centinelas que llegan en
 * el mismo tic se encolan en el orden en que aparecen en el archivo.
 */
static int cmp_arrival(const void *pa, const void *pb)
{
    const arrival_t *a = pa;
    const arrival_t *b = pb;

    if (a->arrivalTime != b->arrivalTime)
        return a->arrivalTime < b->arrivalTime ? -1 : 1;
    if (a->line != b->line)
        return a->line < b->line ? -1 : 1;
    return 0;
}

/* Comparador por ID, usado para detectar IDs duplicados. */
static int cmp_id(const void *pa, const void *pb)
{
    const arrival_t *a = pa;
    const arrival_t *b = pb;

    if (a->id != b->id)
        return a->id < b->id ? -1 : 1;
    return cmp_arrival(pa, pb);
}

/*
 * Revisa que no haya IDs repetidos. Se ordena una copia por ID para que los
 * repetidos queden juntos: O(n log n) en vez de comparar todos con todos.
 */
static int check_duplicates(const char *path, const scenario_t *sc)
{
    arrival_t *copy;
    size_t i;
    int rc = 0;

    if (sc->count < 2)
        return 0;

    copy = malloc(sc->count * sizeof *copy);
    if (copy == NULL) {
        fprintf(stderr, "Error: memoria insuficiente\n");
        return -1;
    }
    memcpy(copy, sc->items, sc->count * sizeof *copy);
    qsort(copy, sc->count, sizeof *copy, cmp_id);

    for (i = 1; i < sc->count; i++) {
        if (copy[i].id == copy[i - 1].id) {
            fprintf(stderr,
                    "Error: %s:%ld: ID duplicado %d (ya aparece en la "
                    "linea %ld)\n", path, copy[i].line, copy[i].id,
                    copy[i - 1].line);
            rc = -1;
            break;
        }
    }
    free(copy);
    return rc;
}

int scenario_load(const char *path, scenario_t *sc)
{
    FILE *f;
    char *line = NULL;
    size_t linecap = 0;
    size_t capacity = 0;
    long lineno = 0;
    int rc = 0;

    sc->items = NULL;
    sc->count = 0;

    f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "Error: no se pudo abrir '%s': %s\n",
                path, strerror(errno));
        return -1;
    }

    /* getline reserva y agranda el búfer según el largo de cada línea. */
    while (getline(&line, &linecap, f) != -1) {
        arrival_t a;

        lineno++;
        if (is_blank_or_comment(line))
            continue;
        if (parse_line(path, lineno, line, &a) != 0) {
            rc = -1;
            break;
        }

        /* Arreglo dinámico: se duplica la capacidad cuando se llena. */
        if (sc->count == capacity) {
            size_t newcap = capacity == 0 ? 16 : capacity * 2;
            arrival_t *tmp = realloc(sc->items, newcap * sizeof *tmp);
            if (tmp == NULL) {
                fprintf(stderr, "Error: memoria insuficiente\n");
                rc = -1;
                break;
            }
            sc->items = tmp;
            capacity = newcap;
        }
        sc->items[sc->count++] = a;
    }

    if (rc == 0 && ferror(f)) {
        fprintf(stderr, "Error: no se pudo leer '%s'\n", path);
        rc = -1;
    }
    free(line);
    fclose(f);

    if (rc == 0 && sc->count == 0) {
        fprintf(stderr, "Error: '%s' no contiene ningun centinela\n", path);
        rc = -1;
    }
    if (rc == 0)
        rc = check_duplicates(path, sc);

    if (rc != 0) {
        scenario_free(sc);
        return -1;
    }

    /* Las llegadas pueden venir desordenadas: se ordenan por tiempo. */
    qsort(sc->items, sc->count, sizeof *sc->items, cmp_arrival);
    return 0;
}

void scenario_free(scenario_t *sc)
{
    free(sc->items);
    sc->items = NULL;
    sc->count = 0;
}
