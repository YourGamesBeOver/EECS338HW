#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/wait.h>

//changing these constants will change the way the program behaves
//how many threads are spawned (there will be an additional deposit thread spawned after these ones for cleanup purposes)
#define NUMBER_OF_THREADS 10
// deposit threads will deposit $1 - $MAX_DEPOSIT, selected randomly
#define MAX_DEPOSIT 750
// withdraw threads will withdraw $1 - $MAX_WITHDRAWL, selected randomly
#define MAX_WITHDRAWL 750
// the initial balance in the account
#define INITIAL_BALANCE 500

//NOTE: in order to keep my code as portable as possible, I am going to assign each thread a number manually instead of using pthread_self
struct threadparams{
	float amount;
	int tnum;
};

//kees track of all the random threads
pthread_t * threads[NUMBER_OF_THREADS];
//the final cleanup thread
pthread_t * cleanupThread;

//semaphores
sem_t mutex, withdraw, deposit;


float balance, needed;

//keeps track of how many people are waiting for a withdraw to finish
int withdrawQueueLength = 0;

//forward declares
void sem_init_wrap(sem_t *sem, unsigned int value);
void sem_destroy_wrap(sem_t *sem);
void sem_wait_wrap(sem_t *sem);
void sem_signal_wrap(sem_t *sem);
void pthread_create_wrap(pthread_t * thread, void *(*start_routine) (void *), void *arg);
void pthread_join_wrap(pthread_t * thread);
int randr(int min, int max);
void Withdraw(float amount, int tnum);
void Deposit(float amount, int tnum);
void * BeginWithdraw(void * amount);
void * BeginDeposit(void * amount);


int main(int argc, char *argv[]){
	//initialize the semaphores
	sem_init_wrap(&mutex, 1);
	sem_init_wrap(&withdraw, 0);
	sem_init_wrap(&deposit, 0);
	
	//seed the RNG
	time_t timeval = time(NULL);
	if(timeval < 0){
		perror("Error getting the system time!");
		exit(EXIT_FAILURE);
	}
	srand(timeval);
	
	//misc init
	balance = INITIAL_BALANCE;
	printf("[MAIN] Initial Balance: $%.2f\n", balance);
	needed = 0.0f;
	
	//create the threads
	printf("[MAIN] Generating %d threads...\n", NUMBER_OF_THREADS);
	int i;
	for(i = 0; i < NUMBER_OF_THREADS; i++){
		threads[i] = malloc(sizeof(pthread_t)); //allocate the thread
		struct threadparams * params = malloc(sizeof(struct threadparams)); //allocate space for the parameters
		params->tnum = i; //assign the ID number
		if(randr(0,2) == 1){ //50/50 chance of deposit or withdraw
			params->amount = randr(1, MAX_WITHDRAWL) * 1.0f;
			printf("[MAIN] Generating withdraw thread %d with a value of %.2f\n", i, params->amount);
			pthread_create_wrap(threads[i], BeginWithdraw, (void *)params);
		}else{
			params->amount = randr(1, MAX_DEPOSIT) * 1.0f;
			printf("[MAIN] Generating deposit thread %d with a value of %.2f\n", i, params->amount);
			pthread_create_wrap(threads[i], BeginDeposit, (void *)params);
		}
		sleep(1);
	}
	printf("[MAIN] All threads have been generated, Generating an extra one that will deposit $100000000 to ensure that all withdrawl threads terminate\n");
	
	struct threadparams * params = malloc(sizeof(struct threadparams));
	cleanupThread = malloc(sizeof(pthread_t));
	params->tnum = -1;
	params->amount = 100000000.0f;
	pthread_create_wrap(cleanupThread, BeginDeposit, (void *) params);
	
	
	printf("[MAIN] Done generating threads, now waiting for them to finish...\n");
	for(i = 0; i < NUMBER_OF_THREADS; i++){
		pthread_join_wrap(threads[i]);
		free(threads[i]);
	}
	pthread_join_wrap(cleanupThread);
	free(cleanupThread);
	
	printf("[MAIN] All threads have finished, cleaning up and returning...\n");
	
	sem_destroy_wrap(&mutex);
	sem_destroy_wrap(&withdraw);
	sem_destroy_wrap(&deposit);
	return 0;
}

//the start of withdraw threads
void * BeginWithdraw(void * amount){
	struct threadparams * args = (struct threadparams *)amount;
	Withdraw(args->amount, args->tnum);
	free(amount);
	return 0;
}

void Withdraw(float amount, int tnum){
	printf("[Thread %d] I want to withdraw $%.2f\n", tnum, amount);
	sem_wait_wrap(&mutex);
	if(needed > 0){
		printf("[Thread %d] Somebody is already waiting to withdraw, I must wait\n", tnum);
		withdrawQueueLength++;
		sem_signal_wrap(&mutex);
		sem_wait_wrap(&withdraw);
		sem_wait_wrap(&mutex);
		withdrawQueueLength--;
	}
	if(balance < amount){
		printf("[Thread %d] There is only $%.2f left and I want to withdraw $%.2f!\n", tnum, balance, amount);
		needed = amount - balance;
		printf("[Thread %d] Now $%.2f is needed, I will wait for a deposit\n", tnum, needed);
		sem_signal_wrap(&mutex);
		sem_wait_wrap(&deposit);
		sem_wait_wrap(&mutex);
	}
	balance = balance - amount;
	if(withdrawQueueLength > 0) sem_signal_wrap(&withdraw);
	printf("[Thread %d] I have made my withdrawl, and there is now $%.2f remaining\n", tnum, balance);
	sem_signal_wrap(&mutex);
	printf("[Thread %d] Withdraw finished!\n", tnum);
}

//start of deposit threads
void * BeginDeposit(void * amount){
	struct threadparams * args = (struct threadparams *)amount;
	Deposit(args->amount, args->tnum);
	free(amount);
	return 0;
}

void Deposit(float amount, int tnum){
	printf("[Thread %d] I want to deposit $%.2f\n", tnum, amount);
	sem_wait_wrap(&mutex);
	balance = balance + amount;
	printf("[Thread %d] I have deposited my $%.2f; the new balance is $%.2f\n", tnum, amount, balance);
	if(needed > 0){
		if(amount >= needed){
			needed = 0;
			printf("[Thread %d] signaling deposit...\n", tnum);
			sem_signal_wrap(&deposit);
		} else {
			needed = needed - amount;
		}
		printf("[Thread %d] After my deposit, $%.2f is needed\n", tnum, needed);
	}
	sem_signal_wrap(&mutex);
	printf("[Thread %d] I am now done with my deposit!\n", tnum);
}

//// wrappers for system calls

void sem_init_wrap(sem_t *sem, unsigned int value){
	if(sem_init(sem, 0, value) < 0) {
		perror("sem_init() failed!");
		exit(EXIT_FAILURE);
	}
}
void sem_destroy_wrap(sem_t *sem){
	if(sem_destroy(sem) != 0){
		perror("Failed to destroy semaphore!");
		exit(EXIT_FAILURE);
	}

}
void sem_wait_wrap(sem_t *sem){
	if(sem_wait(sem) != 0) {
		perror("sem_wait() failed!");
		exit(EXIT_FAILURE);
	}
}

void sem_signal_wrap(sem_t *sem){
	if(sem_post(sem) < 0) {
		perror("sem_post() failed!");
		exit(EXIT_FAILURE);
	}
}

void pthread_create_wrap(pthread_t * thread, void *(*start_routine) (void *), void *arg){
	if(pthread_create(thread, NULL, start_routine, arg) != 0) {
		perror("pthread creation failed!");
		exit(EXIT_FAILURE);
	}
}

void pthread_join_wrap(pthread_t * thread){
	if(pthread_join(*thread, NULL) != 0) {
		perror("Failed to join pthread!");
		exit(EXIT_FAILURE);
	}
}
//generates a random integer between min (inclusive), and max (exclusive)
int randr(int min, int max){
	float rnd = (rand() * 1.0f)/RAND_MAX;
	rnd = rnd*(max-min);
	return (int)(rnd + min);
}

