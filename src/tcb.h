/*
 * tcb.h - Bloque de Control de Tarea (TCB) de un centinela.
 *
 * Cada centinela que llega al sistema se representa con un TCB. El TCB guarda
 * todo el contexto que el planificador necesita para decidir quién usa la CPU
 * y para calcular las métricas al final de la simulación.
 */
#ifndef TCB_H
#define TCB_H

/* Número de colas de prioridad del planificador (0, 1 y 2). */
#define NUM_QUEUES 3

/*
 * Estados de ejecución de un centinela.
 *   NEW         recién creado por el hilo maestro, todavía no está en una cola.
 *   READY       esperando en su cola por primera vez o tras rotar por quantum.
 *   RUNNING     es el centinela activo en CPU_0.
 *   INTERRUPTED fue expropiado por una cola superior y espera en la cabeza de
 *               su cola para continuar con el quantum que le quedaba.
 *   FINISHED    completó toda su ráfaga.
 */
typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_INTERRUPTED,
    STATE_FINISHED
} tcb_state_t;

/*
 * Bloque de Control de Tarea.
 *
 * Los tiempos usan long para que escenarios largos no desborden un int.
 * El campo next permite enlazar el TCB dentro de una cola (lista enlazada
 * intrusiva): un centinela nunca está en dos colas a la vez, así que basta
 * con un solo puntero y no hace falta reservar nodos aparte.
 */
typedef struct tcb {
    int         id;             /* identificador único del centinela          */
    tcb_state_t state;          /* estado de ejecución actual                 */
    int         priority;       /* cola a la que pertenece: 0, 1 o 2          */
    long        arrivalTime;    /* tic en que llegó al sistema                */
    long        totalBurst;     /* ráfaga total de CPU requerida              */
    long        remainingBurst; /* ráfaga que le falta por ejecutar           */
    long        usedQuantum;    /* quantum consumido en el turno actual       */
    long        finishTime;     /* tic en que terminó (Tg + 1 del último tic) */
    long        returnTime;     /* tiempo de retorno: finishTime - arrival    */
    long        waitTime;       /* tiempo de espera: returnTime - totalBurst  */
    struct tcb *next;           /* siguiente TCB en la cola (o NULL)          */
} tcb_t;

/* Reserva e inicializa un TCB en estado NEW. Devuelve NULL si no hay memoria. */
tcb_t *tcb_create(int id, int priority, long arrivalTime, long totalBurst);

/* Libera un TCB creado con tcb_create. */
void tcb_destroy(tcb_t *t);

/* Nombre del estado, para las trazas del modo verbose. */
const char *tcb_state_name(tcb_state_t s);

/* Nombre del tipo de cola tal como aparece en la salida: Command, Driller, Heavy. */
const char *queue_type_name(int priority);

/*
 * Quantum de cada cola: 2 para la cola 0, 4 para la cola 1 y 0 para la
 * cola 2. El valor 0 significa "sin quantum" (FCFS puro).
 */
long queue_quantum(int priority);

#endif /* TCB_H */
