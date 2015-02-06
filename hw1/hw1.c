//Steven Miller
//slm115
//EECS338
//HW1
//hw1.c

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

void printCWD(void);
void printCuser(void);
int spawnChild(int childNumber);
void doChildActions(int childNumber);
int nextFib(void);
void printUIDs(void);
void printTimeUsage(void);
void endProcess(void);

char *cuserid(char *string);//I shouldn't need this forward declaration, but gcc gives warnings if I don't for some reason.  

char *id = "[PARENT]";

int f1 = 1;
int f2 = 1;
int fn = 2;//what number in the sequence f2 represents

int myTurn = 0;

int main(){
	printCWD();
	printCuser();
	printUIDs();

	printf("Let\'s try some forks...\n");
	printf("\n");		
	printf("%s Forking first child...\n",id);
	
	int forkstatus1 = spawnChild(0);
	if(forkstatus1<0)return forkstatus1;
	if(forkstatus1==0)return 0;
	
	printf("%s Forking second child...\n",id);

	int forkstatus2 = spawnChild(1);
	if(forkstatus2 < 0)return forkstatus2;
	if(forkstatus2 == 0)return 0;

	printf("%s Waiting for children to terminate...\n",id);

	int childrenRemaining = 2;
	do{
		printf("%s Children remaining: %d\n",id,childrenRemaining);
		int status;
		int childpid = wait(&status);
		printf("%s A child terminated!\n",id);
		if(WIFEXITED(status)){
			printf("%s Child with pid %d terminated with exit status %d.\n",id,childpid,WEXITSTATUS(status));
			childrenRemaining--;
		}else {
			char errormessage[50];
			sprintf("%s Child with pid %d did NOT terminate normally!",id,childpid);
			perror(errormessage);
			childrenRemaining--;
		}

	}while(childrenRemaining>0);

	printf("%s No children remaining, terminating...\n",id);
	endProcess();
	return 0;

}

void doChildActions(int childNumber){
	sleep(5);

	if(childNumber==0){
		printf("\n%s I calculate that F_%d is %d.\n",id,1,f1);
		myTurn = 1;
	} else {
		sleep(1);
		printf("%s I calculate that F_%d is %d.\n",id,2,f2);
		myTurn = 0;
	}
	while(fn<10){
		sleep(2);
		
		int next = nextFib();
		if(myTurn){
			printf("%s I calculate that F_%d is %d.\n",id,fn,next);
			myTurn = 0;
		}else{
			myTurn = 1;
		}
	}
	sleep(2);//let the other sibiling finish up
	
	endProcess();
}

//moves us ahead by one element in the fib sequence and returns it
int nextFib(){
	int f3 = f1+f2;
	f1 = f2;
	f2 = f3;
	fn++;
	return f3;
}

int spawnChild(int childNumber){
	int cpid = fork();

	if(cpid<0){
		perror("Failed to fork first child!\n");
		return -1;
	} else if (cpid == 0){
		char newIdStr[10];
		sprintf(newIdStr,"[CHILD %d]", childNumber);
		id = newIdStr;
		pid_t pid = getpid();
		pid_t ppid = getppid();
		printf("%s Hello! I\'m Child %d, my process ID is %d, my parent\'s is %d\n",id, childNumber, pid, ppid);
		printUIDs();
		doChildActions(childNumber);
	} else {
		printf("%s Child process successfully spawned with PID %d\n",id, cpid);
	}
	return cpid;
}


void printCWD(){
	char *cwd = getcwd(NULL,0);
	printf("%s Current working directory is %s\n",id, cwd);
	free(cwd);

}

void printCuser(){
	char cuser[16];//15 characters should be enough, right?
	cuserid(cuser);
	if(cuser[0]!='\0')
		printf("%s Current user is %s\n",id, cuser);
	else
		perror("There was an error getting the current user.\n");	
}

void printUIDs(){
	uid_t uid = getuid();
	uid_t euid = geteuid();
	gid_t gid = getgid();
	gid_t egid = getegid();
	printf("%s I am currently running under user ID %d, in group %d, and effectively running under user %d in group %d.\n",id,uid,gid,euid,egid);


}

void printTimeUsage(){
	struct tms buf = {0};
	if(times(&buf)<0){
		perror("There was an error getting the time stats");
		return;
	}
	clock_t utime = buf.tms_utime;
	clock_t stime = buf.tms_stime;
	printf("%s User time used: %ju, System time used: %ju.\n",id, (uintmax_t)utime, (uintmax_t)stime);


}

void endProcess(){
	//waste time before terminating...
	int i,i2;
	for(i2= 0; i2< 5; i2++)
		for(i = 0; i < 100000000; i++);
	
	time_t t = time(NULL);
	char *timestr = ctime(&t);
	printf("%s Terminating at %s\n",id,timestr);
	printTimeUsage();
	exit(0);

}


