#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "swim_mill.h"

bool reachable(int fish_row, int fish_col, int pellet_row, int pellet_col){
	bool reachable = false;
	int Vdist = fish_row - pellet_row;
	int Hdist = abs(fish_col - pellet_col);
	if(Hdist <= Vdist){
		reachable = true;
	}
	return reachable;
}

int main(int argc, char *args[]){
	//declarations
	int fish = 0x0F;
	int pellet = 0x80;
	int row_pos[20];
	int col_pos[20];
	int loop_max = 100; //for testing
	int loop_count = 0; //for testing
	int pellet_count = 1; //starts at one, fish coords stored at 0 in array
	int prev_col = 0;
	int move = 0;
	int middle = COLS/2;
	int semid;
	struct sembuf bufs[3];


	//grab shmid and semid from arguments array
	char* shm_id = args[1];
	//printf("fish shmid: %s\n", shmid);
	printf("fish SEM_KEY: %d\n", SEM_KEY);

	//convert shmid back to int
	char *conv_ptr;
	int shmid_int;
	shmid_int = strtol(shm_id, &conv_ptr, 10);

	//get semaphore set
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
	
	//spawn fish at center
	sm_Ptr[ROWS-1][COLS/2] = fish;

	//test pellets
	//sm_Ptr[7][2] = pellet;
	//sm_Ptr[6][2] = pellet;
	//sm_Ptr[5][2] = pellet;

	//print initial array
	printf("Initial swim_mill array from fish\n");
	for(int i = 0; i < ROWS; i++){
		printf("|");
		for(int j = 0; j < COLS; j++){
			printf("%02X", sm_Ptr[i][j]);
			printf("|");
		}
		printf("\n");
	}

	while(1){ // && loop_count < loop_max
		//get position of fish
		for(int i = 0; i < COLS; i++){
			if(sm_Ptr[9][i] == fish || sm_Ptr[9][i] == 0x8F){
				row_pos[0] = 9;
				col_pos[0] = i;
			}
		}
		pellet_count = 1;
		//printf("fish col: %d\n", col_pos[0]);

		//scan entire array for pellets starting at row above fish
		for(int i = ROWS-2; i >= 0; i--){
			for(int j = 0; j < COLS; j++){
				if (sm_Ptr[i][j] == pellet){
					row_pos[pellet_count] = i;
					col_pos[pellet_count] = j;
					pellet_count++;
				}
			}
		}

		/*
		//print fish position
		printf("Fish: [%d,%d]\n", row_pos[0], col_pos[0]);

		//print pellet positions
		for(int i = 1; i < pellet_count; i++){
			printf("Pellet %d: [%d,%d]\n", i, row_pos[i], col_pos[i]);
		}
		*/

		//determining where fish should move
		prev_col = col_pos[0];
		move = 0;
		//if no pellets are found, move back toward center
		if(pellet_count < 2){
			//printf("No pellets found\n");
			move = middle - col_pos[0];
			if(move < 0){
				col_pos[0] -= 1;
			}
			else if(move > 0){
				col_pos[0] += 1;
			}
		}
		//check if closest pellet is reachable, if so move towards it, if not check next pellet
		for(int i = 1; i < pellet_count; i++){
			//printf("Evaluating pellet at [%d][%d]\n", row_pos[i], col_pos[i]);
			if(reachable(row_pos[0], col_pos[0], row_pos[i], col_pos[i])){
				//printf("Pellet is reachable\n");
				move = col_pos[0] - col_pos[i];
				//move fish one space right
				if(move < 0){
					col_pos[0] += 1;
					//printf("fish moved one lane right\n");
					break;
				}
				//move fish one space left
				else if(move > 0){
					col_pos[0] -= 1;
					//printf("fish moved one lane left\n");
					break;
				}
				//dont't move
				else if(move == 0){
					//printf("fish stayed in lane\n");
					break;
				}
			}
			else{
				//printf("pellet was not reachable\n");
			}
		}
		
		//update fish position
		printf("about to update fish position\n");
		//printf("fish semid: %d\n", semid);
		//set semaphore values, fish needs +1 to fire 
		bufs->sem_num = 0;
		bufs->sem_op = -1;
		bufs->sem_flg = SEM_UNDO;
		//check if able to enter shared memory, block if not
		if(semop(semid, bufs, 2) == -1){
			perror("fish tries to reserve");
			exit(EXIT_FAILURE);
		}
		printf("fish is editing shared memory\n");
		sm_Ptr[ROWS-1][prev_col] = 0;
		sm_Ptr[row_pos[0]][col_pos[0]] |= 0x0F;

		/*
		//add 2 to release first pellet
		bufs->sem_num = 0;
		bufs->sem_op = 2;
		bufs->sem_flg = SEM_UNDO;
		if(semop(semid, bufs, 2) == -1){
			perror("fish tries to release");
			exit(EXIT_FAILURE);
		}*/
		
		//print array
		/*
		printf("Printing swim_mill array from fish in loop\n");
		for(int i = 0; i < ROWS; i++){
			printf("|");
			for(int j = 0; j < COLS; j++){
				printf("%02X", sm_Ptr[i][j]);
				printf("|");
			}
			printf("\n");
		}*/
		
		loop_count++;
	}

	//print final array from fish, will never get here because 
	//fish looks for pellets until killed so for testing only
	printf("Final swim_mill array from fish\n");
	for(int i = 0; i < ROWS; i++){
		printf("|");
		for(int j = 0; j < COLS; j++){
			printf("%02X", sm_Ptr[i][j]);
			printf("|");
		}
		printf("\n");
	}

	//detach shared memory
	if(shmdt(sm_Ptr) == -1){
		perror("shmdt");
		exit(EXIT_FAILURE);
	}

	//return status; successful execution of program
	printf("fish is exiting\n");
	exit(EXIT_SUCCESS);
}