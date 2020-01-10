//by Viktwria Biliouri  AEM 2138 and h Kyriaki Chiona  AEM 2129
//used 2 message queues for the implementation below
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include<sys/time.h>

#define MAX_SIZE 64

volatile sig_atomic_t flag = 0;

//struct me mhnuma kai tupo mhnumatos
typedef struct msg_t {
	long mtype;
	char data[MAX_SIZE];
}msg;

//kanei to flag=1 pou shmainei eimaste monoi mas an erthei otan flag=1 shmainei oti mphke o allos opote kanei flag=0
void handler(int signum){
    if (signum == SIGUSR1){
        if (flag == 0){
            flag = 1;
        }else if(flag == 1){
            flag = 0;
        }
    }
} 

//metatrepei se long ena string
unsigned long hash(char *str)
{
    unsigned long hash = 5381;
    int c;

    c = *(str++);
    
    while (c) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        c = *(str++);
    }

    return hash;
}

//arxikopoihsh ouras mhnumatwn
int msgq_init(unsigned long key, int flag) {
    
    int msgqid;
    
    //dhmiourgias ouras mhnumatwn me to key pou dw8hke
    msgqid = msgget(key, IPC_CREAT|IPC_EXCL|S_IRWXU);
    
    if ((msgqid < 0) && (errno != EEXIST)) {     //apetuxe h shmget 
        fprintf(stderr, "Error in msgget: %s\n", strerror(errno));
        exit(1);
    }
    else if(msgqid < 0){    //exei hdh dhmiourgh8ei h oura eimaste sto 2o treksimo
        
        msgqid = msgget(key, S_IRWXU);
        
        if (msgqid < 0){
            fprintf(stderr, "Error in msgget2: %s\n", strerror(errno));
            exit(1);
        }
    }
    
    return(msgqid);
}

//stelnei to mhnuma sthn oura mhnumatwn
void send(int id, msg message){
    
    int return_value;
    
    return_value = msgsnd(id, &message, sizeof(msg), 0);
    
    if (return_value < 0) {
        fprintf(stderr, "Error in msgsnd: %s\n", strerror(errno));
        exit(1);
    }
    
}
//paralamvanei to mhnuma apo thn oura mhnumatwn
void receive(int id, msg* message){
    int return_value;
    return_value = msgrcv(id, message, sizeof(msg), message->mtype, 0);
    if (return_value < 0) {
        fprintf(stderr, "Error in msgrcv: %s\n", strerror(errno));
        exit(1);
    }
}

//katastrefei thn oura mhnumatwn
void msgq_destroy(int msgqid) {
    
    int return_value;
    
    return_value = msgctl(msgqid, IPC_RMID, 0);
    if (return_value != 0) {
        fprintf(stderr, "Error in msgctl: %s\n", strerror(errno));
        exit(1);
    }
    
    
}
//kai apo dw h main mas. main pes "geia" :D
int main (int argc, char *argv[]) {
    
    pid_t pid, ppid;
    char name[100], mesg[64];
    unsigned long senderKey, receiverKey;
    long senderId, receiverId;
    char *ret_val;
    msg msg;
    
    //elegxos orismatwn
    if (strcmp(argv[2],"2")){
		exit(1);
	}	
	if (argc != 4){
            fprintf(stderr,"Error in number of arguments\n");
            exit(1);
    }
    
    //antigrafh onomatwn se ena onoma gia paragwgh kleidiwn
    strcpy( name, argv[1] );
    strcat( name, argv[3] );
    senderKey = hash( &name[0] );
    //printf("%s  %ld\n",name, senderKey);
    memset(name,0,100);
    strcpy( name, argv[3] );
    strcat( name, argv[1] );
    receiverKey = hash( &name[0] );
    //printf("%s %ld\n",name, receiverKey);
    
    senderId =  msgq_init(senderKey, O_WRONLY );
    receiverId =  msgq_init(receiverKey,O_RDONLY);
    
    
    //dhmiourgia diergasiwn
    pid = fork();
    if (pid == -1 ){
        fprintf(stderr,"Error in fork\n");
        exit(1);
    }
    
    if (pid == 0 ){     //paidi ////receiverId
        //paidi pairnei kai typwnei
        
        //an message == quit stelnei shma ston patera SIGUSR1 oti termatisan. ( PID patera = getppid())
        while (1) {
            msg.mtype = receiverKey;
            receive(receiverId, &msg);
            if (strcmp(msg.data, "quit\n") == 0) {
                
                //stelnei SIGUSR1 ston patera
                ppid = getppid();
                kill(ppid, SIGUSR1);
            }
            else {
                fprintf(stderr, "%s: %s", argv[3], msg.data);
            }
            if (flag == 1 ) {
                //ksanampike o allos xristis epomenws prepei na steiloume sigusr1 sima gia na allaksei o pateras to flag
                ppid = getppid();
                kill(ppid, SIGUSR1);
            }
        }
    }
    else {              //pateras diavazei kai stelnei //senderId
        while(1) {//vlepw shma
            
            signal(SIGUSR1, handler);       
            
            ret_val = fgets(mesg, MAX_SIZE-1, stdin);
            if (ret_val == NULL){
                fprintf(stderr, "Error in fgets: %s\n",strerror(errno));
                exit(1);
            }

            //senderId -> send quit
            if (strcmp(mesg, "quit\n") == 0 ){
                strcpy(msg.data, mesg);
                msg.mtype = senderKey;
                
                send(senderId, msg);   //stelnei quit sto paidi tou allou xristh
                kill(0, SIGTERM);   //skotwnei to paidi tou
                break;
            }else{
                strcpy(msg.data, mesg);
                msg.mtype = senderKey;
                send(senderId, msg);
            }
            
            
            
        }
        //katastrofh ourwn mhnumatwn
        if (flag == 1){
            msgq_destroy(senderId);
            msgq_destroy(receiverId);
        }
        
    }
    
    
    return(0);
}
