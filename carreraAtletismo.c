// SISTEMAS OPERATIVOS AI2 RubenServando

#include "carreraAtletismo.h"

const int comp_tot = 100;       // limitar un máximo de competidores

typedef struct Competidor {     // estructura para array
    int distancia;              // distancia del array (carrera)
    int pos_id;                 // id de la posicion del proceso (competidor)
} Competidor;

typedef struct  Monitor {
    int ganador;                        // variable para el ganador
    int num_competidores;               // variable para num de competidores
    sem_t mutex;                        // semaforo para sincronizar la memoria compartida
    Competidor competidores[comp_tot];  // limitamos el array a un max de competidores
} Monitor;

// funcion para calcular la distancia entre ecompetidores
int calcular_distancia(const void *p1, const void *p2) {
    return ((Competidor *)p1)->distancia - ((Competidor *)p2)->distancia;
}

int main(){
    int shmid;
    Monitor *monitor;
    pid_t pos_id;
    int i, num_competidores, long_carrera, distancia_reccorrida;

    // obtener datos por consola (num competidores)
    printf("Introduzca un número de competidores: ");
    scanf("%d", &num_competidores);

    while (num_competidores > comp_tot || num_competidores <= 0) {
        printf("Número erróneo. Debe introducir un entero entre 1 y %d\n", comp_tot);
        printf("Introduzca un número de competidores: ");
        scanf("%d", &num_competidores);
    }

    // obtener datos por consola (distancia)
    printf("introduzca la longitud de la carrera: ");
    scanf("%d", &long_carrera);

    // crear la zona de memoria compartida
    if ((shmid = shmget(IPC_PRIVATE, sizeof(Monitor), IPC_CREAT | 0666)) < 0) {

        perror("shmget");
        exit(1);
    }

    // mapear la zona de memoria compartida
    if ((monitor = shmat(shmid, NULL, 0)) == (Monitor *) -1) {
        perror("shmat");
        exit(1);
    }

    // inicializar el monitor con los datos introducidos por consola
    monitor->num_competidores = num_competidores;
    monitor-> ganador = -1;
    for (i=0; i < num_competidores; i++) {

        monitor->competidores[i].pos_id = 0;
        monitor->competidores[i].distancia = 0;
    }

    // crear semáforo para controlar el acceso a la memoria compartida
    if ((monitor->mutex = sem_open("semaphore", O_CREAT, 0644, 1)) == SEM_FAILED) {
      perror("sem_open");
      exit(EXIT_FAILURE);
    }

    // crear procesos hijos para cada competidor
    for (i=0; i < num_competidores; i++) {

        pos_id = fork();
        if(pos_id < 0) {

            perror("fork");
            exit(1);
        }
        else if (pos_id == 0) {
            // codigo del proceso hijo (competidor)
            while (monitor->ganador == -1) {

                // adquirir el semáforo antes de modificar la memoria compartida
                sem_wait(&monitor->mutex);

                // calcular la distancia recorrida en esta vuelta (rango entre 1 - 2 m)
                distancia_reccorrida = rand() % 2 + 1;
                monitor->competidores[i].distancia += distancia_reccorrida;
                monitor->competidores[i].pos_id = getpid();

                // comprobar si el competidor ha llegado a recorrido la longitud total (meta)
                if (monitor->competidores[i].distancia >= long_carrera) {

                    monitor->ganador = i;
                }
                else {

                    qsort(monitor->competidores, num_competidores, sizeof(Competidor), calcular_distancia);
                }

                // liberar el semáforo después de modificar la memoria compartida
                sem_post(&monitor->mutex);

                // dormir el proceso
                usleep(1000);
            }

            exit(0);
        }
    }

    // esperar a que los procesos hijos terminan (competidores)
    while (wait(NULL) > 0);

    // imprimir los resultados de la carrera. 1-> el primer proceso terminado (ganador)
    printf("\n***************** Resultado de la Competición *****************\n");
    printf("\nNúmero de competidores: %d corredores | Carrera de: %d metros\n", num_competidores, long_carrera);
    printf("\nEl ganador es: #%d\n", monitor->competidores[monitor->ganador].pos_id);
    
    // imprimimos. 2-> la información del resto de procesos (competidores)
    for (i=0; i < num_competidores; i++ ) {

        printf("El competidor #%d recorre: %d metros\n", monitor->competidores[i].pos_id, monitor->competidores[i].distancia);
    }

    // finalizar el semáforo mutex
    sem_close(&monitor->mutex);

    // eliminar el semáforo
    sem_unlink("semaphore");
    
    // desasgrupar la memoria compartida
    shmdt(monitor);

    // liberar la memoria compartida
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}