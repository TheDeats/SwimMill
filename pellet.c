#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "swim_mill.h"

int main(int argc, char *args[]){
	char* shmid = args[1];
	//printf("pellet shmid: %s\n", shmid);
	int pellet_pid = getpid();
	struct sembuf bufs[3];
	int semid;
	//pellet semaphore value
	//get next val between 0-3, fish fires at one so pellets start at 2, hence the +2
	//finally multiple by negative 1 to make negative
	int sem_val = ((pellet_pid % 4) + 2) * -1;

	//get shmid and convert back to int
	char *conv_ptr;
	int shmid_int;
	shmid_int = strtol(shmid, &conv_ptr, 10);
	//printf("shmid_int: %d\n", shmid_int);

	semid = semget(SEM_KEY, 0, 0);
	if(semid == -1){
		perror("semid");
		exit(EXIT_FAILURE);
	}

	//attach shared memory
	int (*sm_Ptr)[ROWS];
	sm_Ptr = shmat(shmid_int, NULL, 0);
	if (sm_Ptr == (void *) -1){
		perror("shmat");
		exit(EXIT_FAILURE);
	}

	//spawn a pellet randomly in swim_mill
	int pellet = 0x80;
	srand(pellet_pid);
	int r_row = rand() % 9;
	int r_col = rand() % 10;
	sm_Ptr[r_row][r_col] = pellet;
	//printf("Spawning pellet here: [%d][%d]\n", r_row, r_col);

	/*
	//print swim_mill 
	printf("Printing swim_mill array from pellet pid: %d\n", pellet_pid);
	for(int i = 0; i < ROWS; i++){
		printf("|");
		for(int j = 0; j < COLS; j++){
			printf("%02X", sm_Ptr[i][j]);
			printf("|");
		}
		printf("\n");
	}
	*/

	//move pellet through swim_mill
	for(int i = r_row; i < ROWS-1; i++){
		//set semaphore values, pellet needs +2 thru +6 to fire 
		bufs->sem_num = 0;
		bufs->sem_op = sem_val;
		bufs->sem_flg = SEM_UNDO;
		//check if able to enter shared memory, block if not
		if(semop(semid, bufs, 2) == -1){
			perror("pellet tries to reserve");
			exit(EXIT_FAILURE);
		}
		sm_Ptr[r_row][r_col] = 0;
		r_row += 1;
		sm_Ptr[r_row][r_col] |= 0x80;
		/*
		//fire next pellet
		bufs->sem_op = sem_val+1;
		if(semop(semid, bufs, 2) == -1){
			perror("pellet tries to release");
			exit(EXIT_FAILURE);
		}*/
	}

	//check if pellet was consumed and remove pellet
	if(sm_Ptr[r_row][r_col] == 0x8F){
		//printf("pellet was consumed\n");
		sm_Ptr[r_row][r_col] = 0x0F;
	}

	//pellet has reached end of swim mill so removing it from swim_mill array
	sm_Ptr[r_row][r_col] = 0;

	//detach shared memory
	if(shmdt(sm_Ptr) == -1){
		perror("shmdt");
		exit(EXIT_FAILURE);
	}

	//return status; successful execution of program
	printf("pellet %d is exiting\n", getpid());
	exit(EXIT_SUCCESS);
}