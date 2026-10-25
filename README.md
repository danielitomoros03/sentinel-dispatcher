# sentinel-dispatcher

Proyecto 2 de Sistemas Operativos (6004, UCV, I-2026): simulador de un planificador
monoprocesador por tics con colas multinivel apropiativas (RR Q=2, RR Q=4, FCFS),
implementado en C99/POSIX con hilo maestro (reloj) e hilo trabajador (CPU_0).

- Enunciado: [`docs/enunciado.pdf`](docs/enunciado.pdf)
- Informe técnico: [`docs/informe.md`](docs/informe.md)

## Uso

```
make
./sentinel-dispatcher [-v|--verbose] --input inputs/sion_attack.txt
```

| Objetivo | Qué hace |
|---|---|
| `make` | Compila y produce `./sentinel-dispatcher` |
| `make test` | Corre todos los escenarios de `inputs/` y las entradas inválidas de `inputs/invalid/` |
| `make informe` | Genera `docs/informe.pdf` desde `docs/informe.md` (requiere pandoc y Chromium; `CHROME=<ruta>` si no se llama `chromium`) |
| `make zip CI="CI1 CI2 CI3"` | Arma `Proyecto2_CI1_CI2_CI3.zip` para la entrega |
| `make clean` | Borra objetos y binario |

## Estructura

```
Makefile
src/main.c          argumentos (getopt_long) y arranque
src/input.c/.h      carga, validación y orden del escenario
src/tcb.c/.h        TCB, estados y parámetros por cola
src/queue.c/.h      cola FIFO enlazada (push_back, push_front, pop_front)
src/scheduler.c/.h  fases del tic, despacho y métricas
src/sim.c/.h        hilo maestro, hilo trabajador y barrera
src/log.c/.h        salida estándar y verbose
inputs/             escenarios con su .expected.txt
inputs/invalid/     entradas inválidas con el mensaje de error esperado
tests/run_tests.sh  script de make test
docs/               enunciado e informe
```

## Integrantes

- Daniel Moros, C.I. 30035269
- Yoseanni Bermudez, C.I. 30347186
- Donander Huertas, C.I. 31518315

## Uso de Inteligencia Artificial

En este proyecto se usó IA (Claude, de Anthropic). El detalle
está en la sección de metodología del informe técnico.
