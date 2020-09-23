void SequentialSort(void);
void CompareLow(int bit);
void CompareHigh(int bit);
int ComparisonFunc(const void * a, const void * b);
#include <stdio.h>    
#include <math.h>
#include <time.h>  
#include <stdlib.h>

#define MASTER 0        // il risultato finale lo fa questo core
#define ELEMENTI_IN_OUTPUT 10   // numero di elementi in output

double inizio_tempo, fine_tempo;
int process_rank, numeri_core;
int * array, dimensione_array;

int main(int argc, char * argv[]) {
    
    int i, j;

    MPI_Init(&argc, &argv); // prende parametri
    MPI_Comm_size(MPI_COMM_WORLD, &numeri_core);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    dimensione_array = atoi(argv[1]) / numeri_core;  // argomento passato in terminale, atoi(argv[1]) sono 
    // i numeri di steps che vengono divisi e danno un lavoro bilanciato per ogni core.. viene inizializzato l'array per salvare numeri random

    array = (int *) malloc(dimensione_array * sizeof(int));

    /* 
    qui si generano numeri casuali per essere poi ordinati,
    viene fatto senza il master che manda numeri random ad
    ogni alro core
    */
    srand(time(NULL));
    for (i = 0; i < dimensione_array; i++) {
        array[i] = rand() % (atoi(argv[1]));
    }

    //metti una barrier finche' tutti i processi hanno finito di generare
    MPI_Barrier(MPI_COMM_WORLD);


    int dimensions = (int)(log2(numeri_core));

    // inizia timer prima dell'ordinamento 
    if (process_rank == MASTER) {
        printf("Numero di core disponibili: %d\n", numeri_core);
        inizio_tempo = MPI_Wtime();
    }


    qsort(array, dimensione_array, sizeof(int), ComparisonFunc);

    for (i = 0; i < dimensions; i++) {
        for (j = i; j >= 0; j--) {

            if (((process_rank >> (i + 1)) % 2 == 0 && (process_rank >> j) % 2 == 0)   // process_rank deve essere pari ed indice j del processo e' 0
             || ((process_rank >> (i + 1)) % 2 != 0 && (process_rank >> j) % 2 != 0))  // process_rank e' dispari ed indice j del processo e' 1
                {
                CompareLow(j);
            } else {
                CompareHigh(j);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (process_rank == MASTER) {
        fine_tempo = MPI_Wtime();

        printf("Elementi in ordine\n");

        for (i = 0; i < dimensione_array; i++) {
            if ((i % (dimensione_array / ELEMENTI_IN_OUTPUT)) == 0) {
                printf("%d ",array[i]);
            }
        }
        printf("\n\n");
        printf("Tempo di esecuzione in millisecondi %f\n", (fine_tempo - inizio_tempo) *1000);
    }

    free(array);

    MPI_Finalize();
    return 0;
}


int ComparisonFunc(const void * a, const void * b) {
    return ( * (int *)a - * (int *)b );
}

    /*

    High: trasferisce dati ai processi accoppiati, riceve i dati e inizia il merge dai dati piu' alti finche'
    si hanno n elementi, dove n e' la dimensione originale dell'array

    Low: come il high solo che inizia il merge dai dati piu' bassi

    */
void CompareLow(int j) {
    int i, min;

    /*
    manda il piu' grande della lista e restituisce il piu' piccolo

    manda intero array a processo H

    */
    int contatore = 0;
    int * buffer_send = malloc((dimensione_array + 1) * sizeof(int));
    MPI_Send( &array[dimensione_array - 1], 1,  MPI_INT, process_rank ^ (1 << j), 0, MPI_COMM_WORLD);

    //ricevi nuovo min di numeri ordinati
    int recv_counter;
    int * buffer_recieve = malloc((dimensione_array + 1) * sizeof(int));
    MPI_Recv(&min,1,MPI_INT,  process_rank ^ (1 << j), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // buffer di tutti i valore che sono maggiori del min mandato da H processi
    for (i = 0; i < dimensione_array; i++) {
        if (array[i] > min) {
            buffer_send[contatore + 1] = array[i];
            contatore++;
        } else {
            break;     
        }
    }

    buffer_send[0] = contatore;

    // manda parte dei dati al buffer
    MPI_Send(buffer_send,contatore,MPI_INT, process_rank ^ (1 << j), 0,MPI_COMM_WORLD );

    // riveci dai dal buffer
    MPI_Recv(buffer_recieve,dimensione_array, MPI_INT, process_rank ^ (1 << j), 0,MPI_COMM_WORLD, MPI_STATUS_IGNORE );

    // prendi valori dal buffer che sono minori del massimo attuale
    for (i = 1; i < buffer_recieve[0] + 1; i++) {
        if (array[dimensione_array - 1] < buffer_recieve[i]) {
            array[dimensione_array - 1] = buffer_recieve[i];
        } else {
            break;     
        }
    }

qsort(array, dimensione_array, sizeof(int), ComparisonFunc);

    // libera il buffer
    free(buffer_send);
    free(buffer_recieve);

    return;
}


void CompareHigh(int j) {
    int i, max;

    // riceve massimo dal array, L.. in buffer
    int recv_counter;
    int * buffer_recieve = malloc((dimensione_array + 1) * sizeof(int));
    MPI_Recv(&max,1,MPI_INT,process_rank ^ (1 << j),0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // manda minimo all' array
    int contatore = 0;
    int * buffer_send = malloc((dimensione_array + 1) * sizeof(int));
    MPI_Send(&array[0],1,MPI_INT,process_rank ^ (1 << j),0,MPI_COMM_WORLD );

    // passa nel buffer una lista di valore che sono piu' piccoli del max
    for (i = 0; i < dimensione_array; i++) {
        if (array[i] < max) {
            buffer_send[contatore + 1] = array[i];
            contatore++;
        } else {
            break;  
        }
    }

    // ricevi blocchi maggiori di min dal cliente
    MPI_Recv(buffer_recieve,dimensione_array, MPI_INT, process_rank ^ (1 << j),0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    recv_counter = buffer_recieve[0];

    // manda partizione al cliente
    buffer_send[0] = contatore;
    MPI_Send(buffer_send, contatore,MPI_INT,process_rank ^ (1 << j),0,MPI_COMM_WORLD);

    // prendi valori del buffer dal processo L che sono maggiori del min corrente
    for (i = 1; i < recv_counter + 1; i++) {
        if (buffer_recieve[i] > array[0]) {
            array[0] = buffer_recieve[i];
        } else {
            break;     
        }
    }

   qsort(array, dimensione_array, sizeof(int), ComparisonFunc);
    free(buffer_send);
    free(buffer_recieve);

    return;
}

