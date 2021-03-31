#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "a2_helper.h"

pthread_t *t4 = NULL;
pthread_t *t3 = NULL;

pthread_mutex_t t4_started_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t4_started_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t t3_started_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t t3_started_cond = PTHREAD_COND_INITIALIZER;

pthread_t *p2th;
pthread_t *p5th;
pthread_t *p7th;
int *id2, *id5, *id7;

int t54end;
int t21end;

int t21canstart = 0;
int t52canstart = 0;

void P(int sem_id, int sem_no)
{
    struct sembuf op = {sem_no, -1, 0};

    semop(sem_id, &op, 1);
}

void V(int sem_id, int sem_no)
{
    struct sembuf op = {sem_no, +1, 0};

    semop(sem_id, &op, 1);
}

void *p2_thread_routine(void *args){
    int id = *((int *)args);
    if(id == 4){
        //t4 waits for t3 to start
        pthread_mutex_lock(&t3_started_mutex);
        while(t3 == NULL){
            pthread_cond_wait(&t3_started_cond,&t3_started_mutex);
        }
        pthread_mutex_unlock(&t3_started_mutex);
    }

    if(id == 1){
        P(t54end,0);
    }

    info(BEGIN,2,id);
    if(id == 4){
        pthread_mutex_lock(&t4_started_mutex);
        t4 = &p2th[3];
        pthread_cond_signal(&t4_started_cond);
        pthread_mutex_unlock(&t4_started_mutex);
    }
    if(id == 3){
        pthread_mutex_lock(&t3_started_mutex);
        t3 = &p2th[2];
        pthread_cond_signal(&t3_started_cond);
        pthread_mutex_unlock(&t3_started_mutex);

        //t3 waits for t4 to end
        pthread_mutex_lock(&t4_started_mutex);
        //check if p4 started
        while(t4 == NULL){
            pthread_cond_wait(&t4_started_cond,&t4_started_mutex);
        }
        pthread_join(*t4,NULL);
        pthread_mutex_unlock(&t4_started_mutex);
    }
    if(id == 1){
        V(t21end,0);
    }
    info(END,2,id);

    return NULL;
}

int p7_active_threads = 0;
pthread_mutex_t limited5_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t limited5_cond = PTHREAD_COND_INITIALIZER;

int count = 0;
int t15died = 0;

pthread_mutex_t running5_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t running5_cond = PTHREAD_COND_INITIALIZER;


void *p7_thread_routine(void *args){
    int id = *((int *)args);

    if(pthread_mutex_lock(&limited5_mutex) != 0){
        //some error
    }
    while(p7_active_threads >= 5){ //already 5 threads active
        if(pthread_cond_wait(&limited5_cond,&limited5_mutex) != 0){
            //some error
        }
    }
    p7_active_threads++;
    if(pthread_mutex_unlock(&limited5_mutex) != 0){
        //some error
    }

    //some action of thread
    info(BEGIN,7,id);
   
    pthread_mutex_lock(&running5_mutex);
    count++;
    while(t15died == 0 && count < 5){
        pthread_cond_wait(&running5_cond,&running5_mutex);
    }
    while(id == 15 && count < 5){
        pthread_cond_wait(&running5_cond,&running5_mutex);
    }
    pthread_mutex_unlock(&running5_mutex);

    pthread_mutex_lock(&running5_mutex);
    
    if(id == 15){
        t15died = 1;
    }
    count--;
    pthread_cond_signal(&running5_cond);
    pthread_mutex_unlock(&running5_mutex);

    info(END,7,id);
    //end of safe area
    if(pthread_mutex_lock(&limited5_mutex) != 0){
        //some error
    }
    p7_active_threads--;
    if(pthread_cond_signal(&limited5_cond) != 0){
        //some error
    }
    if(pthread_mutex_unlock(&limited5_mutex) != 0){
        //some error
    }
    return NULL;
}


void *p5_thread_routine(void *args){
    int id = *((int *)args);
    if(id == 2){
        P(t21end,0);
    }
    info(BEGIN,5,id);
    
    info(END,5,id);
    if(id == 4){
        V(t54end,0);
    }
    return NULL;
}

int main(int argc, char* argv[]){
    init();

    info(BEGIN, 1, 0);
    t21end = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (t21end < 0) {
        perror("Error creating the semaphore set");
        exit(4);
    }

    semctl(t21end, 0, SETVAL, 1);

    t54end = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    if (t54end < 0) {
        perror("Error creating the semaphore set");
        exit(4);
    }

    semctl(t54end, 0, SETVAL, 1);

    int p[10]; //this will hold the pids of the processes
    p[0] = p[1] = 0; //these will not be used
    p[2] = fork();
    switch(p[2]){
        case -1:
            //error message
            break;
        case 0:
            //inside p2

            info(BEGIN,2,0);
            //create threads of p2
            p2th = (pthread_t *)malloc(4*sizeof(pthread_t));
            id2 = (int *)malloc(4*sizeof(int));
            for(int i = 0 ; i < 4 ; i++){
                id2[i] = i+1;
                if(pthread_create(&p2th[i],NULL,p2_thread_routine,&id2[i]) != 0){
                        printf("Could not create threads in process 2");
                        exit(1);
                }
            }
            for(int i = 0 ; i < 4 ; i++){
                pthread_join(p2th[i],NULL);
            }
            
            p[5] = fork();
            switch(p[5]){
                case -1:
                    //error message
                    break;
                case 0:
                    //inside p5
                    info(BEGIN,5,0);
                    p5th = (pthread_t *)malloc(4*sizeof(pthread_t));
                    id5 = (int *)malloc(4*sizeof(int)); 
                    for(int i = 0 ; i < 4 ; i++){
                        id5[i] = i+1;
                        if(pthread_create(&p5th[i],NULL,p5_thread_routine,&id5[i]) != 0){
                            printf("Could not create threads for process 5");
                            exit(1);
                        }
                    }
                    for(int i = 0 ; i < 4 ; i++){
                        pthread_join(p5th[i],NULL);
                    }
                    
                    info(END,5,0);
                    exit(0); //end p5
                    break;
                default:
                    //inside p2 again
                    p[6] = fork();
                    switch(p[6]){
                        case -1:
                            //error message
                            break;
                        case 0:
                            //inside p6
                            info(BEGIN,6,0);
                            p[7] = fork();
                            switch(p[7]){
                                case -1:
                                    //error message
                                    break;
                                case 0:
                                    //inside p7

                                    info(BEGIN,7,0);
                                    
                                    p7th = (pthread_t *)malloc(45*sizeof(pthread_t));
                                    id7 = (int *)malloc(45*sizeof(int));
                                    for(int i = 0 ; i < 45 ; i++){
                                        id7[i] = i+1;
                                        if(pthread_create(&p7th[i],NULL,p7_thread_routine,&id7[i]) != 0){
                                            printf("Could not create threads in process 7");
                                            exit(1);
                                        }
                                    }
                                    for(int i = 0 ; i < 45 ; i++){
                                        if(pthread_join(p7th[i],NULL) != 0){
                                            printf("JOIN ERROR");
                                            exit(4);
                                        }
                                    }
                                    info(END,7,0);
                                    exit(0);
                                    break;
                                default:
                                    //inside p6
                                    //p6 waits for p7
                                    waitpid(p[7],NULL,0);
                                    info(END,6,0);
                                    exit(0);
                                    break;
                            }
                            break;
                        default:
                            //inside p2 again
                            //wait for p5
                            waitpid(p[5],NULL,0);
                            //wait for p6
                            waitpid(p[6],NULL,0);
                            info(END,2,0);
                            exit(0);
                            break;
                    }
                    break;
                }
        default:
            //inside p1 again
            p[3] = fork();
            switch(p[3]){
                case -1:
                    //error message
                    break;
                case 0:
                    //inside p3
                    info(BEGIN,3,0);
                    p[4] = fork();
                    switch(p[4]){
                        case -1:
                            //error message
                            break;
                        case 0:
                            //inside p4
                            info(BEGIN,4,0);
                            //eventual action of p4
                            info(END,4,0);
                            exit(0);
                            break;
                        default:
                            //again in p3
                            p[8] = fork();
                            switch(p[8]){
                                case -1:
                                    //error message
                                    break;
                                case 0:
                                    //inside p8
                                    info(BEGIN,8,0);
                                    //action of p8
                                    info(END,8,0);
                                    exit(0);
                                    break;
                                default:
                                    //inside p3 again
                                    //p3 waits for p4
                                    waitpid(p[4],NULL,0);
                                    //p3 waits for p8
                                    waitpid(p[8],NULL,0);
                                    info(END,3,0);
                                    exit(0);
                                    break;
                            }
                            break;
                    }
                    break;
                default:
                    //inside p1 again
                    p[9] = fork();
                    switch(p[9]){
                        case -1:
                            //error message
                            break;
                        case 0:
                            //inside p9
                            info(BEGIN,9,0);
                            //action of p9
                            info(END,9,0);
                            exit(0);
                            break;
                        default:
                            //inside p1 again
                            //p1 waits for p2 and p3 and p9
                            waitpid(p[3],NULL,0);
                            waitpid(p[9],NULL,0);
                            waitpid(p[2],NULL,0);
                            info(END,1,0);
                            exit(0);
                            break;
                    }
                    break;
            }
            break;
    }
    pthread_mutex_destroy(&t3_started_mutex);
    pthread_mutex_destroy(&t4_started_mutex);
    pthread_cond_destroy(&t3_started_cond);
    pthread_cond_destroy(&t4_started_cond);
    free(p2th);
    free(id2);

    pthread_mutex_destroy(&limited5_mutex);
    pthread_cond_destroy(&limited5_cond);
    free(id7);
    free(p7th);

    free(p5th);
    free(id5);

    semctl(t21end, 0, IPC_RMID, 0);
    semctl(t54end, 0, IPC_RMID, 0);
    return 0;
}
