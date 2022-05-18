#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <signal.h>
#include "swim_mill.h"

int max_pellets = 4;
int process_count = 1;
pid_t pellet_pids[4];
pid_t fish_pid;

void decrement_processes(int sig_num){
	process_count--;
	//fflush(stdout);
	//printf("Sig_num: %d\n", sig_num);
	printf("process count decremented: %d\n", process_count);
}

void sig_kill_children(int sig_num){
	signal(SIGINT, sig_kill_children);
	printf("PID: %d CTRL-C was entered, killing all children\n", getpid());
	kill(fish_pid, SIGKILL);
	for(int i = 0; i < max_pellets; i++){
		kill(pellet_pids[i], SIGKILL);
	}
	//fflush(stdout);
	printf("All children have been killed, exiting\n");
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]){

	//signal handling
	signal(SIGCHLD, decrement_processes);
	signal(SIGINT, sig_kill_children);

	//declarations
	int total_wait_time = 5;
	int elapsed_time = 0;
	int fish = 0x0F;
	int pellet = 0x80;
	int total_loops = 0;
	int max_loops = 50;
	int pellets_created = 0;
	int pellet_array_slot = 0;
	char semkey_str[6];
	char shmid_str[7];
	union semun sem_arg, dummy;
	struct sembuf bufs[3];
	clock_t start_time = clock();
	//printf("start time: %ld\n", start_time);

	//create swim_mill array
	int swim_mill[ROWS][COLS];
	//pointer to swim_mill array in shared memory
	int (*sm_Ptr)[ROWS];
	int semid, shmid, bytes, xfrs;
	//struct shmseg *shmp;

	//creates a set of semaphores for the pellets and fish
	semid = semget(SEM_KEY, 1, IPC_CREAT | OBJ_PERMS);
	if(semid == -1){
		perror("semget");
		exit(EXIT_FAILURE);
	}
	printf("parent semid: %d\n", semid);
	//convert semid KEY to string to pass as argument to other processes
	
	sprintf(semkey_str, "%d", SEM_KEY);
	printf("parent semkey_str: %s\n", semkey_str);

	//try to initialize to 1 to allow fish to execute first
	sem_arg.val = 0;
	if(semctl(semid, 0, SETVAL, sem_arg) == -1){
		perror("initial semctl");
		exit(EXIT_FAILURE);
	}

	//creating shared memory space
	shmid = shmget(SHM_KEY, sizeof(swim_mill), IPC_CREAT | OBJ_PERMS);
	if (shmid == -1){
		perror("shmget");
		exit(EXIT_FAILURE);
	}
	//convert shmid to string to pass as argument to other processes
	printf("parent shmid: %d\n", shmid);
	sprintf(shmid_str, "%d", shmid);
	//printf("shmid_str: %s\n", shmid_str);

	//attaching shared memory address space
	sm_Ptr = shmat(shmid, NULL, 0);
	if (sm_Ptr == (void *) -1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}

	//set everything to 0 in swim mill array in shared memory
	for(int i = 0; i < ROWS; i++){
		for(int j = 0; j < COLS; j++){
			sm_Ptr[i][j] = 0;
		}
	}

	//kick off fish process, only stops for timer and interrupt
	char* fish_args[] = {"fish", shmid_str, NULL}; 
	fish_pid = fork();
	//execute fish process
	if(fish_pid == -1){
		perror("fish fork");
		exit(EXIT_FAILURE);
	}
	else if(fish_pid == 0){
		execv("./fish.o", fish_args);
	}
	process_count++;

	//pellets continuously spawned based on timer and process count
	char* pellet_args[] = {"pellet", shmid_str, NULL};
	elapsed_time = (clock() - start_time) / CLOCKS_PER_SEC;
	while(elapsed_time < total_wait_time){ //elapsed_time < total_wait_time || total_loops < max_loops
		//printf("elapsed time: %d second(s)\n", elapsed_time);
		//printf("total loops: %d\n", total_loops);
		//printf("process count: %d\n", process_count);
		//printf("total pellets: %d\n", total_pellets);
		if(process_count < max_pellets + 2){
			pellet_array_slot = pellets_created % max_pellets;
			pellet_pids[pellet_array_slot] = fork();
			//execute pellet process
			if(pellet_pids[pellet_array_slot] == -1){
				perror("pellet fork");
				exit(EXIT_FAILURE);
			}
			if(pellet_pids[pellet_array_slot] == 0){
				execv("./pellet.o", pellet_args);
			}
			printf("pellet pid created: %d\n", pellet_pids[pellet_array_slot]);
			process_count++;
			pellets_created++;
		}
		/*if(process_count >= max_pellets + 2){
			if(wait(&pellet_status) == -1){
				perror("pellet wait");
				exit(1);
			}
		}*/

		//fire processes starting with fish
		bufs->sem_num = 0;
		bufs->sem_op = (total_loops % 5) + 1;
		bufs->sem_flg = SEM_UNDO;
		if(semop(semid, bufs, 2) == -1){
			perror("parent semop loop");
			exit(EXIT_FAILURE);
		}
		//printf("parent semop, num: %d\n", bufs->sem_op);

		elapsed_time = (clock() - start_time) / CLOCKS_PER_SEC * 2;
		total_loops++;

		//printf("finished loop\n");
	}
	//printf("elapsed time: %d second(s)\n", elapsed_time);

	while(elapsed_time < total_wait_time){
		elapsed_time = (clock() - start_time) / CLOCKS_PER_SEC; 
	}
	printf("process count: %d\n", process_count);

	//kill pellets
	printf("Trying to kill pellets\n");
	for(int i = 0; i < max_pellets; i++){
		kill(pellet_pids[i], SIGKILL);
		/*if(wait(&pellet_status) == -1){
			perror("pellet kill");
			exit(1);
		}*/
	}
	printf("Pellets were successfully killed\n");

	//kill fish
	printf("Trying to kill fish\n");
	kill(fish_pid, SIGKILL);
	printf("Fish was successfully killed\n");

	//remove semaphore set
	if(semctl(semid, 0, IPC_RMID, dummy) == -1){
		perror("final semctl");
		exit(EXIT_FAILURE);
	}

	//detach shared memory
	if(shmdt(sm_Ptr) == -1){
		perror("shmdt");
		exit(EXIT_FAILURE);
	}
	
	//delete shared memory segment
	if (shmctl(shmid, IPC_RMID, 0) == -1){
		perror("shmctl");
		exit(EXIT_FAILURE);
	}

	//return status; successful execution of program
	exit(EXIT_SUCCESS);
}
