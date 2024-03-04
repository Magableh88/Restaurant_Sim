/**
 * Starter code for the restaurant simulation
 * Author: Pranjal Patra
*/

//Mohammad Makableh
//Student ID: 202130654
//Grace days used: 2 
//Grace days left: 7

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFER_SIZE 20
#define BURGER 'B'
#define FRIES 'F'
#define SODA 'S'
#define EMPTY '_'
#define ONE_SECOND 1000000 // set this to 1000000 for 1 second; Adjust this value to speed up or slow down the simulation


// Global variables to store the shared memory segment IDs and pointers
// Your code here [1]

int *numCustomers; // variable to keep track of customers
int shm_id_burger, shm_id_fries, shm_id_soda, shm_id_customer; 
struct Queue *burger_queue, *fries_queue, *soda_queue;

/**
 * Structure to represent a queue of items
 * 
 * The buffer is a circular array of characters.
 * The in and out variables are used to keep track of the position of the next item to be added or removed.
 * The num_produced variable is used to keep track of the number of items produced.
 * 
 * The buffer is considered full when in == out - 1 (mod BUFFER_SIZE)
 * The buffer is considered empty when in == out
 * 
 * Example:
 * buffer: [_, B, B, _, _, _, _, _, _, _]
 * in: 3
 * out: 1
 * num_produced: 3
 * 
 * This means that the next item to be added will be at position 3, and the next item to be removed will be at position 1.
*/
struct Queue {
    char buffer[BUFFER_SIZE];
    int in;
    int out;
    int num_produced;
};


// Function prototypes
void initialize_queue(struct Queue *queue);
void produce(char item, int interval, struct Queue *queue, int quantity);
void consume(int burgers_needed, int fries_needed, int sodas_needed);
void monitor_queues();
void cleanup();
void signal_handler(int sig);


/**
 * Main function
 * 
 * @param argc The number of command-line arguments
 * @param argv An array of command-line argument strings
 */
int main(int argc, char *argv[]) {
    setbuf(stdout, NULL); // Disable output buffering

    printf("================================================================================\n");

    int totalBurgersRequired = 0;
    int totalFriesRequired = 0;
    int totalSodasRequired = 0;

    if (argc < 2) {
        printf("Usage: %s sequence_of_customers\n", argv[0]);
        printf("Example: %s I F I P\n", argv[0]);
        exit(1);
    }
    else {
        // display command entered
        printf("Command entered: ");
        for (int i = 1; i < argc; i++) {
            printf("%s ", argv[i]);
            if (argv[i][0] == 'I') {
                totalBurgersRequired += 1;
                totalFriesRequired += 1;
                totalSodasRequired += 1;
            }
            else if (argv[i][0] == 'F') {
                totalBurgersRequired += 4;
                totalFriesRequired += 6;
                totalSodasRequired += 3;
            }
            else if (argv[i][0] == 'G') {
                totalBurgersRequired += 3;
                totalFriesRequired += 4;
                totalSodasRequired += 2;
            }
            else if (argv[i][0] == 'P') {
                totalBurgersRequired += 8;
                totalFriesRequired += 12;
                totalSodasRequired += 16;
            }
            else {
                printf("Invalid customer type: %s\n", argv[i]);
                exit(1);
            }
        }
        printf("\n");
        printf("Total Customers: %d, Total Burgers Required: %d, Total Fries Required: %d, Total Sodas Required: %d\n", argc-1, totalBurgersRequired, totalFriesRequired, totalSodasRequired);
    }


    // Create shared memory segments for burger, fries, and soda queues
    // Your code here [2]

    //creating shared memory segment for burger
    shm_id_burger = shmget(IPC_PRIVATE, sizeof(struct Queue), IPC_CREAT | 0666);
    if (shm_id_burger == -1) {
        perror("shmget failed");
        exit(1); 
    }

    //creating shared memory segment for fries
    shm_id_fries = shmget(IPC_PRIVATE, sizeof(struct Queue), IPC_CREAT | 0666);
    if (shm_id_fries == -1) {
        perror("shmget failed");
        exit(1); 
    }

    //creating shared memory segment for soda    
    shm_id_soda = shmget(IPC_PRIVATE, sizeof(struct Queue), IPC_CREAT | 0666);
    if (shm_id_soda == -1) {
        perror("shmget failed");
        exit(1); 
    }

    //creating shared memory segment for customer
    shm_id_customer = shmget(IPC_PRIVATE, sizeof(struct Queue), IPC_CREAT | 0666);
    if (shm_id_customer == -1) {
        perror("shmget failed");
        exit(1); 
    }

    // Attach to the shared memory segments
    // Your code here [3]

    burger_queue = (struct Queue*)shmat(shm_id_burger, NULL, 0);
    if (burger_queue == (void *)-1) { //checks for an error when attaching the queue to the memory segment
        exit(1);
    }

    fries_queue = (struct Queue*)shmat(shm_id_fries, NULL, 0);
    if (fries_queue == (void *)-1) { //checks for an error when attaching the queue to the memory segment
        exit(1);
    }

    soda_queue = (struct Queue*)shmat(shm_id_soda, NULL, 0);
    if (soda_queue == (void *)-1) { //checks for an error when attaching the queue to the memory segment
        exit(1);
    }

    numCustomers = (int *)shmat(shm_id_customer, NULL, 0);
    if (numCustomers == (void *)-1) { //checks for an error when attaching the queue to the memory segment
        exit(1);
    }


    // Initialize the queues and shared memory segments
    // Like this: initialize_queue(burger_queue); must set the in and out to 0 and the buffer to EMPTY
    // Your code here [4]

    *numCustomers = argc-1; //asigning number of customers based on number of arguments given
    initialize_queue(burger_queue);
    initialize_queue(fries_queue);
    initialize_queue(soda_queue);

    // Fork a process to monitor the queues and display the current state of the buffer 
    // for each item type every second by calling the monitor_queues function
    // Your code here [5]

    if(fork() == 0){
        printf("Queues monitor process started.\n");
        monitor_queues();
        printf("All customers have been served. Exiting.\n");
        exit(0);
    }


    // Fork 3 producer processes for burgers, fries, and sodas
    // Each process will produce items at a different interval by calling the produce function
    // Your code here [6]

    if(fork() == 0){
        printf("Burger queue process started\n");
        produce(BURGER, 4, burger_queue, 2);
        exit(0);
    }

    if(fork() == 0){
        printf("Fries queue process started\n");
        produce(FRIES, 5, fries_queue, 3);
        exit(0);
    }

    if(fork() == 0){
        printf("Soda queue process started\n");
        produce(SODA, 1, soda_queue, 1);
        exit(0);
    }

    /** Process each customer type in the argument list, one per second
     *
     * Example:
     * ./restaurant I F G P
     * 
     * This will process the following customers:
     * 1. Individual customer
     * 2. Family customer
     * 3. Group customer
     * 4. Party customer
     * 
     * Each customer will be processed one second apart.
    */
    // Your code here [7]

    for (int i = 1; i < argc; i++) {
        usleep(ONE_SECOND);
        if (fork() == 0) {
            //If statments check which type of customer is being processed at the moment
            if (argv[i][0] == 'I') {
                printf("Individual customer enters at time = %d. Order: 1 burger, 1 fries, and 1 soda.\n", i);
                consume(1, 1, 1); //calls the consume function according to the type of customer
                printf("Inidividual ustomer consumed 1 burger, 1 fries, and 1 soda. Customer count: %d\n", *numCustomers-1);
            }
            else if (argv[i][0] == 'F') {
                printf("Family customer enters at time = %d. Order: 4 burgers, 6 fries, and 3 sodas.\n", i);
                consume(4, 6, 3); //calls the consume function according to the type of customer
                printf("Family customer consumed 4 burgers, 6 fries, and 3 sodas. Customer count: %d\n", *numCustomers-1);
            }
            
            else if (argv[i][0] == 'G') {
                printf("Group customer enters at time = %d. Order: 3 burgers, 4 fries, and 2 sodas.\n", i);
                consume(3, 4, 2); //calls the consume function according to the type of customer
                printf("Group customer consumed 3 burgers, 4 fries, and 2 sodas. Customer count: %d\n", *numCustomers-1);
            }
            
            else if (argv[i][0] == 'P') {
                printf("Party customer enters at time = %d. Requires 8 burgers, 12 fires, and 16 sodas.\n", i);
                consume(8, 12, 16); //calls the consume function according to the type of customer
                printf("Party customer consumed 8 burgers, 12 fires, and 16 sodas. Customer count: %d\n", *numCustomers-1);
            }
            
            //subtracts a customer from the count after they have been served
            *numCustomers -= 1;
            exit(0);
        }
    }
    // Setup signal handling for cleanup
    // Your code here [8]
    
    signal(SIGINT, signal_handler);  // Handle Ctrl+C
    signal(SIGTERM, signal_handler); // Handle termination request

    // wait for all child processes to finish
    // Your code here [9]

    while (wait(NULL) > 0);
    cleanup();
    return 0;
}

/**
* Initialize the buffer to be empty
* @param queue A pointer to the Queue structure to be initialized
* 
* Example: initialize_queue(burger_queue) will initialize the burger queue.
* The in and out will be set to 0 and the buffer will be set to EMPTY.
* The num_produced will be set to 0.
*/
void initialize_queue(struct Queue *queue) {
    // Your code here [10]
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        queue->buffer[i] = EMPTY;
    }
    queue->in = 0;
    queue->out = 0;
    queue->num_produced = 0;
}

/**
 * Produces items at a given interval and stores them in the queue.
 * It also updates the num_produced, in, and out variables accordingly.
 *
 * @param item The item to be produced.
 * @param interval The interval at which the item is produced.
 * @param queue A pointer to the Queue structure where the item will be stored.
 * @param quantity The number of items to produce at once.
 * 
 * Example: produce('B', 4, burger_queue, 2) will produce 2 burgers every 4 seconds.
 * Example: produce('F', 5, fries_queue, 3) will produce 3 fries every 5 seconds.
 */
void produce(char item, int interval, struct Queue *queue, int quantity) {
    // Your code here [11]

    //while loop to keep production going until all customers are served
    while (*numCustomers > 0) {
        usleep(interval*ONE_SECOND);
        for (int i = 0; i <= quantity-1; i++) { 
            // Check if the next position is free before producing
            if (queue->buffer[queue->in] == EMPTY) {
                queue->buffer[queue->in] = item;
                queue->in = (queue->in + 1) % BUFFER_SIZE;
                queue->num_produced++;
            }
            else {
                printf("%c Queue is full. Waiting for a customers to consume items.\n", item);
            }
        }
    }
}


/**
 * Consumes items from the queue at a given interval.
 * It also updates the in and out variables accordingly.
 *
 * @param burgers_needed The number of burgers needed.
 * @param fries_needed The number of fries needed.
 * @param sodas_needed The number of sodas needed.
 * 
 * Example: consume(1, 1, 1) will consume 1 burger, 1 fries, and 1 soda.
 * Example: consume(4, 6, 3) will consume 4 burgers, 6 fries, and 3 sodas.
 */
void consume(int burgers_needed, int fries_needed, int sodas_needed) {
    // Your code here [12]

    int burgers_consumed, fries_consumed, sodas_consumed = 0; //variables to keep track of how many of each item is consumed

    //while loop terminates after the custmoer consumes everything in their order
    while(burgers_needed > burgers_consumed || fries_needed > fries_consumed || sodas_needed > sodas_consumed){
        if(burger_queue ->buffer[burger_queue->out] == BURGER && burgers_needed > 0){
            burger_queue->buffer[burger_queue->out] = EMPTY;
            burger_queue->out = (burger_queue->out+1)%BUFFER_SIZE;
            burgers_consumed++;
        }
        if(fries_queue ->buffer[fries_queue->out] == FRIES && fries_needed > 0){
            fries_queue->buffer[fries_queue->out] = EMPTY;
            fries_queue->out = (fries_queue->out+1)%BUFFER_SIZE;
            fries_consumed++;
        }
        if(soda_queue ->buffer[soda_queue->out] == SODA && sodas_needed > 0){
            soda_queue->buffer[soda_queue->out] = EMPTY;
            soda_queue->out = (soda_queue->out+1)%BUFFER_SIZE;
            sodas_consumed++;
        }
        usleep(ONE_SECOND/2);
    }
}


/**
 * Monitors the queues and displays the current state of the buffer for each item type.
 * This function will print the current state of the queues every second.
 */
void monitor_queues() {
    // Your code here [13]

    int timeCounter = 0; 
    //while loop prints the state of the restaurant every second
    while (*numCustomers > 0) {
        usleep(ONE_SECOND);
        timeCounter++;
        printf("%03d:\t BURGERS: [", timeCounter);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            printf("%c ", burger_queue->buffer[i]);
        }
        printf("] FRIES: [");
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            printf("%c ", fries_queue->buffer[i]);
        }
        printf("] SODAS: [");
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            printf("%c ", soda_queue->buffer[i]);
        }
        printf("]\n");
    }

}


/**
 * Cleanup and exit the program
 * 
 * This function will detach from the shared memory segments and remove them.
 * 
 */
void cleanup() {
    // Your code here [14]

    
    printf("Clean up and terminating\n");

    //if statements check for errors when removing/detaching the shared memory segments
    if (shmdt(burger_queue) == -1) {
        perror("Error detaching from burger_queue");
    }
    
    if (shmdt(fries_queue) == -1) {
        perror("Error detaching from fries_queue");
    }
    
    if (shmdt(soda_queue) == -1) {
        perror("Error detaching from soda_queue");
    }

    if (shmdt(numCustomers) == -1) {
        perror("Error detaching from customers");
    }

    if (shmctl(shm_id_burger, IPC_RMID, NULL) == -1) {
        perror("Error removing burger_queue");
    }
    
    if (shmctl(shm_id_fries, IPC_RMID, NULL) == -1) {
        perror("Error removing fries_queue");
    }
    
    if (shmctl(shm_id_soda, IPC_RMID, NULL) == -1) {
        perror("Error removing soda_queue");
    }
    
    if (shmctl(shm_id_customer, IPC_RMID, NULL) == -1) {
        perror("Error removing customers");
    }
}


/**
 * Signal handler to cleanup and exit the program. 
 * This function will be called when the program receives a signal, such as SIGINT (Ctrl+C)
 * 
 * Example usage:
 * signal(SIGINT, signal_handler);
 * 
 * @param sig The signal number
 */
void signal_handler(int sig) {
    cleanup();
    exit(0);
}
