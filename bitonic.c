void SequentialSort(void);
void CompareLow(int bit);
void CompareHigh(int bit);
int ComparisonFunc(const void * a, const void * b);
#include "mpi.h"
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
Skip to content
Pull requests
Issues
Marketplace
Explore
@linuxguy2020
linuxguy2020 /
mpi

1
0

    0

Code
Issues
Pull requests
Actions
Projects
Wiki
Security
Insights

    Settings

mpi/bitonic.c
@linuxguy2020
linuxguy2020 Create bitonic.c
Latest commit c0e6d61 7 hours ago
History
1 contributor
276 lines (233 sloc) 9.34 KB
#include <stdio.h>      // Printf
#include <time.h>       // Timer
#include <math.h>       // Logarithm
#include <stdlib.h>     // Malloc
#include "mpi.h"        // MPI Library
#include "bitonic.h"

#define MASTER 0        // Who should do the final processing?
#define OUTPUT_NUM 10   // Number of elements to display in output

// Globals
// Not ideal for them to be here though
double timer_start;
double timer_end;
int process_rank;
int num_processes;
int * array;
int array_size;

///////////////////////////////////////////////////
// Main
///////////////////////////////////////////////////
int main(int argc, char * argv[]) {
    int i, j;

    // Initialization, get # of processes & this PID/rank
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    // Initialize Array for Storing Random Numbers
    array_size = atoi(argv[1]) / num_processes;
    array = (int *) malloc(array_size * sizeof(int));

    // Generate Random Numbers for Sorting (within each process)
    // Less overhead without MASTER sending random numbers to each slave
    srand(time(NULL));  // Needed for rand()
    for (i = 0; i < array_size; i++) {
        array[i] = rand() % (atoi(argv[1]));
    }

    // Blocks until all processes have finished generating
    MPI_Barrier(MPI_COMM_WORLD);

    // Begin Parallel Bitonic Sort Algorithm from Assignment Supplement

    // Cube Dimension
    int dimensions = (int)(log2(num_processes));

    // Start Timer before starting first sort operation (first iteration)
    if (process_rank == MASTER) {
        printf("Number of Processes spawned: %d\n", num_processes);
        timer_start = MPI_Wtime();
    }

    // Sequential Sort
    qsort(array, array_size, sizeof(int), ComparisonFunc);

    // Bitonic Sort follows
    for (i = 0; i < dimensions; i++) {
        for (j = i; j >= 0; j--) {
            // (window_id is even AND jth bit of process is 0)
            // OR (window_id is odd AND jth bit of process is 1)
            if (((process_rank >> (i + 1)) % 2 == 0 && (process_rank >> j) % 2 == 0) || ((process_rank >> (i + 1)) % 2 != 0 && (process_rank >> j) % 2 != 0)) {
                CompareLow(j);
            } else {
                CompareHigh(j);
            }
        }
    }

    // Blocks until all processes have finished sorting
    MPI_Barrier(MPI_COMM_WORLD);

    if (process_rank == MASTER) {
        timer_end = MPI_Wtime();

        printf("Displaying sorted array (only 10 elements for quick verification)\n");

        // Print Sorting Results
        for (i = 0; i < array_size; i++) {
            if ((i % (array_size / OUTPUT_NUM)) == 0) {
                printf("%d ",array[i]);
            }
        }
        printf("\n\n");

        printf("Time Elapsed (Sec): %f\n", timer_end - timer_start);
    }

    // Reset the state of the heap from Malloc
    free(array);

    // Done
    MPI_Finalize();
    return 0;
}

///////////////////////////////////////////////////
// Comparison Function
///////////////////////////////////////////////////
int ComparisonFunc(const void * a, const void * b) {
    return ( * (int *)a - * (int *)b );
}

///////////////////////////////////////////////////
// Compare Low
///////////////////////////////////////////////////
void CompareLow(int j) {
    int i, min;

    /* Sends the biggest of the list and receive the smallest of the list */

    // Send entire array to paired H Process
    // Exchange with a neighbor whose (d-bit binary) processor number differs only at the jth bit.
    int send_counter = 0;
    int * buffer_send = malloc((array_size + 1) * sizeof(int));
    MPI_Send(
        &array[array_size - 1],     // entire array
        1,                          // one data item
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD              // default comm.
    );

    // Receive new min of sorted numbers
    int recv_counter;
    int * buffer_recieve = malloc((array_size + 1) * sizeof(int));
    MPI_Recv(
        &min,                       // buffer the message
        1,                          // one data item
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD,             // default comm.
        MPI_STATUS_IGNORE           // ignore info about message received
    );

    // Buffers all values which are greater than min send from H Process.
    for (i = 0; i < array_size; i++) {
        if (array[i] > min) {
            buffer_send[send_counter + 1] = array[i];
            send_counter++;
        } else {
            break;      // Important! Saves lots of cycles!
        }
    }

    buffer_send[0] = send_counter;

    // send partition to paired H process
    MPI_Send(
        buffer_send,                // Send values that are greater than min
        send_counter,               // # of items sent
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD              // default comm.
    );

    // receive info from paired H process
    MPI_Recv(
        buffer_recieve,             // buffer the message
        array_size,                 // whole array
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD,             // default comm.
        MPI_STATUS_IGNORE           // ignore info about message received
    );

    // Take received buffer of values from H Process which are smaller than current max
    for (i = 1; i < buffer_recieve[0] + 1; i++) {
        if (array[array_size - 1] < buffer_recieve[i]) {
            // Store value from message
            array[array_size - 1] = buffer_recieve[i];
        } else {
            break;      // Important! Saves lots of cycles!
        }
    }

    // Sequential Sort
    qsort(array, array_size, sizeof(int), ComparisonFunc);

    // Reset the state of the heap from Malloc
    free(buffer_send);
    free(buffer_recieve);

    return;
}


///////////////////////////////////////////////////
// Compare High
///////////////////////////////////////////////////
void CompareHigh(int j) {
    int i, max;

    // Receive max from L Process's entire array
    int recv_counter;
    int * buffer_recieve = malloc((array_size + 1) * sizeof(int));
    MPI_Recv(
        &max,                       // buffer max value
        1,                          // one item
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD,             // default comm.
        MPI_STATUS_IGNORE           // ignore info about message received
    );

    // Send min to L Process of current process's array
    int send_counter = 0;
    int * buffer_send = malloc((array_size + 1) * sizeof(int));
    MPI_Send(
        &array[0],                  // send min
        1,                          // one item
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD              // default comm.
    );

    // Buffer a list of values which are smaller than max value
    for (i = 0; i < array_size; i++) {
        if (array[i] < max) {
            buffer_send[send_counter + 1] = array[i];
            send_counter++;
        } else {
            break;      // Important! Saves lots of cycles!
        }
    }

    // Receive blocks greater than min from paired slave
    MPI_Recv(
        buffer_recieve,             // buffer message
        array_size,                 // whole array
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD,             // default comm.
        MPI_STATUS_IGNORE           // ignore info about message receiveds
    );
    recv_counter = buffer_recieve[0];

    // send partition to paired slave
    buffer_send[0] = send_counter;
    MPI_Send(
        buffer_send,                // all items smaller than max value
        send_counter,               // # of values smaller than max
        MPI_INT,                    // INT
        process_rank ^ (1 << j),    // paired process calc by XOR with 1 shifted left j positions
        0,                          // tag 0
        MPI_COMM_WORLD              // default comm.
    );

    // Take received buffer of values from L Process which are greater than current min
    for (i = 1; i < recv_counter + 1; i++) {
        if (buffer_recieve[i] > array[0]) {
            // Store value from message
            array[0] = buffer_recieve[i];
        } else {
            break;      // Important! Saves lots of cycles!
        }
    }

    // Sequential Sort
    qsort(array, array_size, sizeof(int), ComparisonFunc);

    // Reset the state of the heap from Malloc
    free(buffer_send);
    free(buffer_recieve);

    return;
}

    © 2020 GitHub, Inc.
    Terms
    Privacy
    Security
    Status
    Help

    Contact GitHub
    Pricing
    API
    Training
    Blog
    About



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

