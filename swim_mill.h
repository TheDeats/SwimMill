#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>

//Key for shared memory segment
#define SHM_KEY 0x1234

//Key for semaphore set
#define SEM_KEY 0x5678

//Permissions for IPC objects
#define OBJ_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)

//number of rows and columns in array
#define ROWS 10
#define COLS 10

//max processes at one time
//#define max_processes 10

//Defines structure of shared memory segment
/*struct shmseg{
    //Number of bytes used in 'swim_mill'
    int cnt;
    ////Create 2D array for swim_mill
    int swim_mill[ROWS][COLS];
};*/

/*
#ifndef SEMBUF_H
#define SEMBUF_H

struct sembufs{
	unsigned short sem_num;
	short sem_op;
	short sem_flg;
};
#endif*/

#ifndef SEMUN_H
#define SEMUN_H

union semun{
    int val;
    struct semid_ds* buf;
    unsigned short* array;
    #if defined(_linux_)
        struct seminfo* _buf;
    #endif
};

#endif