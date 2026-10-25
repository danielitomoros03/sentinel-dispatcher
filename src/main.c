/*
 * main.c - Punto de entrada de sentinel-dispatcher.
 *
 * Proyecto 2 de Sistemas Operativos (6004, UCV, I-2026).
 * Simulador de un planificador monoprocesador por tics con tres colas
 * multinivel apropiativas: RR (Q=2), RR (Q=4) y FCFS.
 *
 * Uso: ./sentinel-dispatcher [-v|--verbose] --input <archivo>
 *
 * Pasos:
 *   1. Leer los argumentos con getopt_long.
 *   2. Cargar y validar el escenario (input.c).
 *   3. Correr la simulación con el hilo maestro y el trabajador (sim.c).
 *   4. Imprimir las métricas globales (scheduler.c).
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "input.h"
#include "log.h"
#include "scheduler.h"
#include "sim.h"

/* Muestra cómo se usa el programa. */
static void usage(FILE *out, const char *prog)
{
    fprintf(out,
            "Uso: %s [-v|--verbose] --input <archivo>\n"
            "  -i, --input <archivo>  escenario con una linea por centinela:\n"
            "                         <llegada> <id> <cola 0-2> <rafaga>\n"
            "  -v, --verbose          traza por tic de colas, despacho y quantum\n"
            "  -h, --help             muestra esta ayuda\n",
            prog);
}

int main(int argc, char *argv[])
{
    static const struct option options[] = {
        { "input",   required_argument, NULL, 'i' },
        { "verbose", no_argument,       NULL, 'v' },
        { "help",    no_argument,       NULL, 'h' },
        { NULL,      0,                 NULL,  0  }
    };
    const char *inputPath = NULL;
    scenario_t  sc;
    scheduler_t s;
    int         opt, rc;

    /* 1. Argumentos. */
    while ((opt = getopt_long(argc, argv, "i:vh", options, NULL)) != -1) {
        switch (opt) {
        case 'i':
            inputPath = optarg;
            break;
        case 'v':
            log_set_verbose(1);
            break;
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        default:
            /* getopt_long ya imprimió qué opción falló. */
            usage(stderr, argv[0]);
            return 2;
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Error: argumento inesperado '%s'\n", argv[optind]);
        usage(stderr, argv[0]);
        return 2;
    }
    if (inputPath == NULL) {
        fprintf(stderr, "Error: falta --input <archivo>\n");
        usage(stderr, argv[0]);
        return 2;
    }

    /* 2. Escenario. scenario_load ya explica el error en stderr. */
    if (scenario_load(inputPath, &sc) != 0)
        return EXIT_FAILURE;

    if (scheduler_init(&s, sc.count) != 0) {
        scenario_free(&sc);
        return EXIT_FAILURE;
    }

    /* 3. Simulación con los dos hilos. */
    log_header();
    rc = sim_run(&s, &sc);

    /* 4. Métricas (solo si la simulación llegó al final). */
    if (rc == 0)
        scheduler_report(&s);

    scheduler_destroy(&s);
    scenario_free(&sc);
    return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
