//by Viktwria Biliouri AEM 2138 and h Kyriaki Chiona  AEM 2129
//1 shared memory used for the below implementation
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include<sys/time.h>

#define MAX_SIZE 65
#define MEM_SIZE 160 //2*65 + 2*15
#define NAME_SIZE 15

volatile sig_atomic_t flag = 0;

void handler(int signum){
            flag = 1;
} 

//sthn 0 8esh ksekinaei to prwto onoma 15  epomeno 30 prwto mhnuma kai 95 to deutero mhnuma
char *shm_init(const char *prog_name, long *shmid, char *name1, char *name2) {
                                                                                                                                                                                                                                                                                                                                                                                                                           
    char *memory;
    unsigned long key;
    
    //dhmiourgia kleidiou apo to onoma tou programmatos
    key = ftok(prog_name, 'N');
    if(key < 0){
        fprintf(stderr,"Error in ftok\n");
        exit(1);
    }
    
    //printf("%ld\n", key);
    
    //dhmiourgia koinoxrhsths mnhmhs kai elegxos an uphrxe h oxi se ka8e treksimo 
    *shmid = shmget(key, MEM_SIZE, IPC_CREAT|IPC_EXCL|S_IRWXU);
    
    if ((*shmid < 0) && (errno != EEXIST)) {     //apetuxe h shmget 
        fprintf(stderr, "Error in shmget: %s\n", strerror(errno));
        exit(1);
    }
    else if(*shmid < 0){    //exei hdh dhmiourgh8ei mnhmh eimaste sto 2o+ tre3imo
        
        *shmid = shmget(key, MEM_SIZE, S_IRWXU);
        
        if (*shmid < 0){
            fprintf(stderr, "Error in shmget2: %s\n", strerror(errno));
            exit(1);
        }
        
        memory = (char *)shmat(*shmid, NULL, 0);
        
        if (*memory < 0) {
            fprintf(stderr, "Error in shmat: %s\n", strerror(errno));
            exit(1);
        }
        //genikoi elegxoi gia swsta onomata sthn mnhmh 
        // check for Eva (oxi idio onoma)
        if (*memory == '\0'){ //an o 1os den yparxei
            
            if (strcmp(memory+NAME_SIZE, name2) != 0) {
                fprintf(stderr, "There is not a receiver online1\n");
                exit(1);
            }
            if (strcmp(memory+1, name1+1) != 0) {
                fprintf(stderr, "Busy. Try later !\n");
                exit(1);
            }
            else {
                strcpy(memory, name1);
            }
        }
        else {              
            if (strcmp(memory, name2) != 0) {
                fprintf(stderr, "There is not a receiver online\n");
                exit(1);
            }
            if (strcmp(memory+1+NAME_SIZE, name1+1) != 0) {     //koitaei to upoloipo tou onomatos
                fprintf(stderr, "Busy. Try later!\n");
                exit(1);
            }
            else {
                strcpy(memory+NAME_SIZE, name1);
            }
        }
        
    }
    else{    //dhmiourgeitai gia prwth fora h mnhmh 1os xrhsths 1o treksimo
        memory = (char *)shmat(*shmid, NULL, 0);
        if (*memory < 0) {
            fprintf(stderr, "Error in shmat: %s\n", strerror(errno));
            exit(1);
        }
        
        if (strcmp(name1, name2) == 0){
            fprintf(stderr, "Forever alone byeee!\n");
            exit(1);
        }
        //arxikopoieitai h koinoxrhsth mnhmh
        strncpy(memory, name1, NAME_SIZE);
        strncpy(memory+NAME_SIZE, name2, NAME_SIZE);
        *(memory + 2*NAME_SIZE) = '\0';
        *(memory + MEM_SIZE-MAX_SIZE) = '\0';
        
    } 
    return(memory);
}

//arxikopoiei tous shmatoforeis pou xrhsimopoioume
int sem_init(const char *prog_name){
    
    unsigned long key;
    int return_value;
    int sem_id;
    
    key = ftok(prog_name, 'N');
    if(key < 0){
        fprintf(stderr,"Error in ftok\n");
        exit(1);
    }
    
    //printf("%ld\n", key);
    
    sem_id = semget(key, 4 , IPC_CREAT|IPC_EXCL|S_IRWXU);
    
    if ((sem_id < 0) && (errno != EEXIST)) {     //apetuxe h shmget 
        fprintf(stderr, "Error in semget: %s\n", strerror(errno));
        exit(1);
        
    }
    
    else if(sem_id < 0){    //exei hdh dhmiourgh8ei 2o programma
        
        sem_id = semget(key, 4, S_IRWXU);
        
        if (sem_id < 0){
            fprintf(stderr, "Error in semget2: %s\n", strerror(errno));
        }
        return (sem_id);
    }
    
    //   arxikopoiei:
    //   s0 = 1 Alice can write
    //   s1 = 0 Bob can read
    //   s2 = 1 Bob can write
    //   s3 = 0 Alice can read
    return_value = semctl(sem_id, 0, SETVAL, 1);
    if (return_value < 0){
        fprintf(stderr, "Error in semctl1: %s\n", strerror(errno));
        exit(1);
    }
    return_value = semctl(sem_id, 1, SETVAL, 0);    
    if (return_value < 0){
        fprintf(stderr, "Error in semctl2: %s\n", strerror(errno));
        exit(1);
    }
    return_value = semctl(sem_id, 2, SETVAL, 1);
    if (return_value < 0){
        fprintf(stderr, "Error in semctl3: %s\n", strerror(errno));
        exit(1);
    }
    return_value = semctl(sem_id, 3, SETVAL, 0);
    if (return_value < 0){
        fprintf(stderr, "Error in semctl4: %s\n", strerror(errno));
        exit(1);
    }
    return (sem_id);

}

//katastrefei tous shmatoforeis
void sem_destroy(int semid) {
    
    int return_value;
    
    return_value = semctl(semid, 0, IPC_RMID);
    if (return_value != 0) {
        fprintf(stderr, "Error in semctl: %s\n", strerror(errno));
        exit(1);
    }
    
    
}



//anevazei tous shmatoforeis 
void up(int sem_id, int sem ) {
    
    int return_value;
    struct sembuf op;
    
    op.sem_num = sem;
    op.sem_op = 1;
    op.sem_flg = 0;
    
    return_value = semop(sem_id, &op,1);
    if (return_value < 0) {
        fprintf(stderr, "Error in semop (up): %s\n", strerror(errno));
        exit(1);
    }
}

//katevazei tous shmatoforeis 
void down(int sem_id, int sem ) {
    
    struct sembuf op;
    
    op.sem_num = sem;
    op.sem_op = -1;
    op.sem_flg = 0;
    
    semop(sem_id, &op,1);
}

//stelnei ta mhnumata(grafei sthn mnhmh) kai anevokatevazei tous antistoixoys shmatoforeis
void send(char *message,int sem_id, int sem_read, int sem_write, char *memory){
    
    down(sem_id, sem_write);
    strcpy(memory, message);
    up(sem_id, sem_read);

}
//paralamvanei ta mhnumata(pairnei apo thn mnhmh) kai anevokatevazei tous antistoixoys shmatoforeis
void receive(char *message,int sem_id, int sem_read, int sem_write, char *memory){
    
    down(sem_id, sem_read);
    strcpy(message, memory);
    up(sem_id, sem_write);
    
}
//kai apo edw h main main pes "geia" :D
int main (int argc, char *argv[]) {
    
    int temp,return_value;
    pid_t pid, ppid; 
    char message[MAX_SIZE];
    struct sigaction action = {{0}};
    long Id,sem_Id;
    char *memory;
    
    //elegxos gia ta orismata
    if (strcmp(argv[2],"2")){
        exit(1);
    }	
    if (argc != 4){
        fprintf(stderr,"Error in number of arguments\n");
        exit(1);
    }
    
    //eisodos shmatos SIGINT ston handler gia na parempodistei h fusiologikh leitourgia tou Ctrl-C
    action.sa_handler = handler;
    return_value = sigaction(SIGINT, &action, NULL);
    if (return_value < 0){
        fprintf(stderr,"Error in sigaction: %s\n", strerror(errno));
        exit(1);
    }
    
    memory = shm_init(argv[0], &Id, argv[1], argv[3]);
    sem_Id = sem_init(argv[0]);
    
    //dhmiourgia diergasiwn
    pid = fork();
    if (pid == -1 ){
        fprintf(stderr,"Error in fork\n");
        exit(1);
    }
    
    if (pid == 0 ){     //paidi ////receiver
        //paidi pairnei kai typwnei
        
        while (1) {
            
            if((strcmp(argv[1],memory)) == 0){ //eimai autos pou dhmiourgise to programma 1os
                receive(message, sem_Id, 1, 0, memory + 2*NAME_SIZE);
                fprintf(stderr, "%s: %s", argv[3], message);
            }else{                              //eimai o 2os ths mnhmhs
                receive(message, sem_Id, 3, 2, memory + 2*NAME_SIZE + MAX_SIZE);
                fprintf(stderr, "%s: %s", argv[3], message);
            }
        }
    }
    else {              //pateras diavazei kai stelnei //sender
        while(1) {
            
            fgets(message, MAX_SIZE-1, stdin);
            message[MAX_SIZE] ='\0';
            
            if(flag == 1){
                break;
            }

            if((strcmp(argv[1],memory)) == 0){ //eimai autos pou dhmiourgise to programma 1os
                send(message, sem_Id, 3, 2, memory + 2*NAME_SIZE + MAX_SIZE);
            }else{                             //eimai o 2os ths mnhmhs
                send(message, sem_Id, 1, 0, memory + 2*NAME_SIZE );
            }
            if (strcmp(message, "quit\n") == 0) {
                break;
            }
            
            
        }
        
     
        //an patise quit h Ctrl-C o 1os xrhsths svhnetai to onoma tou
        if (strcmp(argv[1],memory) == 0){
            memory[0] = '\0';
        }
        //an patise quit h Ctrl-C o 2os xrhsths svhnetai to onoma tou
        else{
            memory[NAME_SIZE] = '\0';
        }
        
        //aposundesh me thn koinoxrhsth mnhmh tou xrhsth pou vriskomaste
        temp = shmdt(memory);
        if (temp < 0){
            fprintf(stderr,"Error in shmdt\n");
            exit(1);
        }
        
        kill(0, SIGTERM);
        
        if ((memory[NAME_SIZE] == '\0') && (memory[0] == '\0')){
                    
            //aposundesh me thn koinoxrhsth mnhmh tou allou xrhsth
            temp = shmdt(memory);
            if (temp < 0){
                fprintf(stderr,"Error in shmdt\n");
                exit(1);
            }
            //apodesmeush koinoxrhsths mnhmhs
            temp = shmctl(Id, IPC_RMID, NULL);        
            if (temp < 0){
                fprintf(stderr,"Error in shmctl\n");
                exit(1);
            }
            //katastrofh shmatoforewn
            sem_destroy(sem_Id);
            
            ppid = getppid();
            kill(ppid, SIGTERM);
                
            return(0);
        }
    }
    return (0);
}

    
