//Steven Miller
//slm115
//eecs338
//HW4

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/times.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <ctype.h>

#define NO_DIRECTION 0
#define EAST_BOUND 1
#define WEST_BOUND -1

#define SEMAPHORE_KEY 0xCAFE

//semaphore array indices
#define SEM_MUTEX 0
#define SEM_WESTBOUND 1
#define SEM_EASTBOUND 2
#define NUMBER_OF_SEMAPHORES 3

//the number of seconds it takes to 'cross' the bridge
#define CROSS_TIME 2

struct shared_variable_struct {
	int XCount, XdCount, EastBoundWaitCount, WestBoundWaitCount, Direction;
};

// This union is required by semctl(2)
// http://linux.die.net/man/2/semctl
union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO
                                (Linux-specific) */
};

int get_semid(key_t semkey);
int get_shmid(key_t shmkey);
void semaphore_wait(int semid, int semnumber);
void semaphore_signal(int semid, int semnumber);
void PrintUsageAndExit(void);
void forkChild(int boundDir, int childNumber);
char* getDirName(int dir);
void doWestBound(int semid, struct shared_variable_struct * svars, char *id);
void doEastBound(int semid, struct shared_variable_struct * svars, char *id);
void genID(char str[], int childNumber, int direction);


int main(int argc, char *argv[]){
	//start off by checking arguments
	if (argc != 2) {
		PrintUsageAndExit();
	}
	
	printf("[PARENT] Hello world, my PID is %d.\n", getpid());
	
	////// make the semaphores
	union semun semaphore_values;

	// Set up our semaphore values according to Tekin's solutions
	unsigned short semaphore_init_values[NUMBER_OF_SEMAPHORES];
	semaphore_init_values[SEM_MUTEX] = 1;
	semaphore_init_values[SEM_WESTBOUND] = 0;
	semaphore_init_values[SEM_EASTBOUND] = 0;
	semaphore_values.array = semaphore_init_values;
	
	//let's make some semaphores!
	int semid = get_semid((key_t)SEMAPHORE_KEY);
	if (semctl(semid, 0, SETALL, semaphore_values) == -1) {
		perror("semctl failed");
		exit(EXIT_FAILURE);
	}
	
	///// time to make the shared variables
	int shmid = get_shmid((key_t)SEMAPHORE_KEY);
	struct shared_variable_struct * shared_variables = shmat(shmid, 0, 0);
	
	shared_variables->XCount = 0;
	shared_variables->XdCount = 0;
	shared_variables->EastBoundWaitCount = 0;
	shared_variables->WestBoundWaitCount = 0;
	shared_variables->Direction = NO_DIRECTION;
	
	//we are now done with the setup, time to get forkin'!
	
	printf("[PARENT] processing arguments: \"%s\"\n\n",argv[1]);
	
	int numberOfChildren = 0;
	int index = 0;
	while(argv[1][index] != 0){
		char cur = argv[1][index];
		if(isdigit(cur)){
			int sleeptime = cur - '0';
			printf("[PARENT] sleeping for %d seconds\n", sleeptime);
			sleep(sleeptime);
		}else if(cur == 'W' || cur == 'w'){
			printf("[PARENT] fork a westbound car!\n");
			forkChild(WEST_BOUND, numberOfChildren);
			numberOfChildren++;
		}else if(cur == 'E' || cur == 'e'){
			printf("[PARENT] fork an eastbound car!\n");
			forkChild(EAST_BOUND, numberOfChildren);
			numberOfChildren++;
		}
		index++;
	}
	
	printf("[PARENT] %d children forked, waiting for them to terminate!\n\n", numberOfChildren);
	
	while(numberOfChildren>0){
		wait(NULL);
		numberOfChildren--;
	}
	
	printf("[PARENT] all children have terminated, cleaning up...\n");
	
	
	
	
	
	// We need to clean up after ourselves
	if (shmdt(shared_variables) == -1) {
		perror("[PARENT] shmdt failed");
		exit(EXIT_FAILURE);
	}

	if (shmctl(shmid, IPC_RMID, NULL) < 0) {
		perror("[PARENT] shmctrl failed");
		exit(EXIT_FAILURE);
	}

	if (semctl(semid, 0, IPC_RMID, semaphore_values) == -1) {
		perror("[PARENT] semctl failed");
		exit(EXIT_FAILURE);
	}
	
	
	return 0;
}

//forks a child that is traveling in the given direction
void forkChild(int boundDir, int childNumber){
	pid_t cpid;
	cpid = fork();
	if(cpid == -1){
		perror("[PARENT] Fork failed");
		exit(EXIT_FAILURE);
	}else if(cpid == 0){
		//we're a child!
		
		//common child setup
		
		char cid[3];
		genID(cid, childNumber, boundDir);
		printf("[%s] Hello, I am a successfully forked %s child, my PID is %d!\n", cid, getDirName(boundDir), getpid());
		int semid = get_semid((key_t)SEMAPHORE_KEY);
		int shmid = get_shmid((key_t)SEMAPHORE_KEY);
		struct shared_variable_struct * svars = shmat(shmid, 0, 0);
		if(boundDir == WEST_BOUND){
			doWestBound(semid, svars, cid);
		} else if(boundDir == EAST_BOUND){
			doEastBound(semid, svars, cid);
		} else {
			printf("[%s] I was given an invalid direction, I don't know where to go :,(\n", cid);
		}
		
		
		//common child cleanup
		if (shmdt(svars) == -1) {
			perror("shmdt failed");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
		
	}else{
		//parent, nothing to do here
		return;
	}
}


void doWestBound(int semid, struct shared_variable_struct * svars, char *id){
	printf("[%s] I have arrived at the bridge and am grabbing the mutex\n", id);
	semaphore_wait(semid, SEM_MUTEX);
	printf("[%s] I have successfully grabbed the mutex and am thinking about crossing the bridge\n", id);
	if((svars->Direction == WEST_BOUND || svars->Direction == NO_DIRECTION) && svars->XCount < 4 && ((svars->XdCount + svars->XCount) < 5 )){
		svars -> Direction = WEST_BOUND;
		svars -> XCount++;
		semaphore_signal(semid, SEM_MUTEX);
	} else {
		svars -> WestBoundWaitCount++;
		semaphore_signal(semid, SEM_MUTEX);
		semaphore_wait(semid, SEM_WESTBOUND);
		svars -> WestBoundWaitCount--;
		svars -> XCount++;
		svars -> Direction = WEST_BOUND;
		semaphore_signal(semid, SEM_MUTEX);
	}
	//crossing time!
	printf("[%s] I am done thinking and am now crossing the bridge!\n", id);
	sleep(CROSS_TIME);
	printf("[%s] I have arrived at the other end of the bridge and am now waiting for the mutex\n", id);
	
	semaphore_wait(semid, SEM_MUTEX);
	printf("[%s] Having successfully grabbed the mutex, I am now thinking about signaling my fellow cars to cross\n", id);
	svars -> XdCount++;
	svars -> XCount--;
	if(svars->WestBoundWaitCount != 0 && (((svars->XdCount + svars->XCount) < 5) || svars->EastBoundWaitCount == 0)){
		semaphore_signal(semid, SEM_WESTBOUND);
	} else if (svars -> XCount == 0 && svars->EastBoundWaitCount != 0 && (svars->WestBoundWaitCount == 0 || (svars->XdCount + svars->XCount) >= 5)){
		svars -> Direction = EAST_BOUND;
		svars -> XdCount = 0;
		semaphore_signal(semid, SEM_EASTBOUND);
	} else if (svars->XCount == 0 && svars->EastBoundWaitCount == 0 && svars->WestBoundWaitCount == 0){
		svars -> Direction = NO_DIRECTION;
		svars -> XdCount = 0;
		semaphore_signal(semid, SEM_MUTEX);
	}else {
		semaphore_signal(semid, SEM_MUTEX);
	}
	printf("[%s] It has been quite the adventure, I think my bridge-crossing days are finally over\n", id);
}

void doEastBound(int semid, struct shared_variable_struct * svars, char *id){
	printf("[%s] I have arrived at the bridge and am grabbing the mutex\n", id);
	semaphore_wait(semid, SEM_MUTEX);
	printf("[%s] I have successfully grabbed the mutex and am thinking about crossing the bridge\n", id);
	if((svars->Direction == EAST_BOUND || svars->Direction == NO_DIRECTION) && svars->XCount < 4 && ((svars->XdCount + svars->XCount) < 5 )){
		svars -> Direction = EAST_BOUND;
		svars -> XCount++;
		semaphore_signal(semid, SEM_MUTEX);
	} else {
		svars -> EastBoundWaitCount++;
		semaphore_signal(semid, SEM_MUTEX);
		semaphore_wait(semid, SEM_EASTBOUND);
		svars -> EastBoundWaitCount--;
		svars -> XCount++;
		svars -> Direction = EAST_BOUND;
		semaphore_signal(semid, SEM_MUTEX);
	}
	//crossing time!
	printf("[%s] I am done thinking and am now crossing the bridge!\n", id);
	sleep(CROSS_TIME);
	printf("[%s] I have arrived at the other end of the bridge and am now waiting for the mutex\n", id);
	
	semaphore_wait(semid, SEM_MUTEX);
	printf("[%s] Having successfully grabbed the mutex, I am now thinking about signaling my fellow cars to cross\n", id);
	svars -> XdCount++;
	svars -> XCount--;
	if(svars->EastBoundWaitCount != 0 && (((svars->XdCount + svars->XCount) < 5) || svars->WestBoundWaitCount == 0)){
		semaphore_signal(semid, SEM_EASTBOUND);
	} else if (svars -> XCount == 0 && svars->WestBoundWaitCount != 0 && (svars->EastBoundWaitCount == 0 || (svars->XdCount + svars->XCount) >= 5)){
		svars -> Direction = WEST_BOUND;
		svars -> XdCount = 0;
		semaphore_signal(semid, SEM_WESTBOUND);
	} else if (svars->XCount == 0 && svars->WestBoundWaitCount == 0 && svars->EastBoundWaitCount == 0){
		svars -> Direction = NO_DIRECTION;
		svars -> XdCount = 0;
		semaphore_signal(semid, SEM_MUTEX);
	}else {
		semaphore_signal(semid, SEM_MUTEX);
	}
	printf("[%s] It has been quite the adventure, I think my bridge-crossing days are finally over\n", id);
}


// These two functions are wrapper functions for the System-V
// style semaphores that were talked about in class. 
// Modified from: http://art.cwru.edu/338.S13/example.semaphore.html
// copied from the solutions for 2013 HW5
void semaphore_wait(int semid, int semnumber) {
	struct sembuf wait_buffer;
	wait_buffer.sem_num = semnumber;
	wait_buffer.sem_op = -1;
	wait_buffer.sem_flg = 0;
	if (semop(semid, &wait_buffer, 1) == -1)  {
		perror("semaphore_wait failed");
		exit(EXIT_FAILURE);
	}
}

void semaphore_signal(int semid, int semnumber) {
	struct sembuf signal_buffer;
	signal_buffer.sem_num = semnumber;
	signal_buffer.sem_op = 1;
	signal_buffer.sem_flg = 0;
	if (semop(semid, &signal_buffer, 1) == -1)  {
		perror("semaphore_signal failed");
		exit(EXIT_FAILURE);
	}
}

// Small wrapper functions to convert the keys of the shared memory
// and the semaphores to values.
// copied from the solutions for 2013 HW5
int get_semid(key_t semkey) {
	int value = semget(semkey, NUMBER_OF_SEMAPHORES, 0777 | IPC_CREAT);
	if (value == -1) {
		perror("semget failed");
		exit(EXIT_FAILURE);
	}
	return value;
}

int get_shmid(key_t shmkey) {
	int value = shmget(shmkey, sizeof(struct shared_variable_struct), 0777 | IPC_CREAT);
	if (value == -1) {
		perror("shmkey failed");
		exit(EXIT_FAILURE);
	}
	return value;
}


void PrintUsageAndExit(){
	printf("Invalid Parameters\n\nusage: main <carlist>\n\n");
	printf("carlist is a string consisting of a combination of the following characters: E, e, W, w, 1, 2, 3, 4, 5, 6, 7, 8, 9\n");
	printf("carlist determines the cars that will be sent to the car, and their timings\n");
	printf("E or e sends an eastbound car and W or w sends a westbound car\n");
	printf("The numbers cause the program to wait for that many seconds before sending the next car\n\n");
	printf("Example: \"main WE2W\" would send a westbound car followed by an eastbound car, then wait two seconds before sending another westbound car\n\n");
	exit(EXIT_SUCCESS);
}

char* getDirName(int dir){
	switch(dir){
	case NO_DIRECTION: return "no direction";
	case WEST_BOUND: return "westbound";
	case EAST_BOUND: return "eastbound";
	default: return "{invaid direction}";
	}
}

//generates the two-char string that identifies each car in the printfs
void genID(char str[], int childNumber, int direction){
	if(direction == WEST_BOUND){
		sprintf(str, "%dW", childNumber);
	} else if(direction == EAST_BOUND){
		sprintf(str, "%dE", childNumber);
	} else {
		sprintf(str, "%dX", childNumber);
	}
}



