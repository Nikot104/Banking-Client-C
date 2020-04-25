#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/time.h>
#include<signal.h> 
#include<pthread.h> 
#include <semaphore.h> 
#include "bankingServer.h"

sem_t mutex; 
int sock_con[256] = {0};
int connection_num = 0;
Account allAccounts[256] = {"",0,0}; 
// This is where I would keep all the account data
//number of accounts in the array
int acc_arr_size = 0; 
pthread_mutex_t lock; 
	
int Create(char* name, int sock)
{
	sem_wait(&mutex); 
	pthread_mutex_lock(&lock); 
	int i = 0;
	for(i = 0; i < acc_arr_size; i ++)
	{
		if(strcmp(allAccounts[i].name, name) == 0)
		{
			char buff[100] = "Error: Account already exists.\n\0";
			printf("Account Creation Error!\n");
			write(sock, buff, strlen(buff));
			return 0;
		}
	}
	
	//setting up new account details
	allAccounts[acc_arr_size].name = strdup(name);
	char client_message[1000];
	sprintf(client_message, "Account created! Name: %s, Balance: $%.2f \n\0",
			name,
			allAccounts[acc_arr_size].balance);
	write(sock, client_message, strlen(client_message));
	acc_arr_size++;
	printf("Account Created!\n");
	pthread_mutex_unlock(&lock); 
	sem_post(&mutex); 
	return 0;
}

void alarm_handler(int sig){
	sem_wait(&mutex);
	int i = 0;
	printf("SERVER DIAGNOSITICS\n");
	printf("========================================================\n");
	for(i = 0; i < acc_arr_size; i ++){
		printf("Account Name: %s\t Balance: $%.2f\t%s",
		allAccounts[i].name, allAccounts[i].balance,
		allAccounts[i].session_flag == 0? "\n": "IN SERVICE\n");
	}
	printf("========================================================\n");
	sem_post(&mutex);
}

void Serve(char* name, int sock){
	int i = 0, found_flag = 0;
	for(i = 0; i < acc_arr_size; i ++){
		if(strcmp(allAccounts[i].name, name) == 0 ){
			if(allAccounts[i].session_flag == 0){
				found_flag = 1;
				char client_message[100] = "service in session. \n\0";
				allAccounts[i].session_flag = 1;
				printf("%s\n",client_message);
				write(sock, client_message, strlen(client_message));
				break;
			}else{
				char client_message[100] = "service is already in session. try again later. \n\0";
				write(sock, client_message, strlen(client_message));
				return;
			}
		}
	}
	if(found_flag == 0){
		char client_message[100] = " account does not exist. \n\0";
		write(sock, client_message, strlen(client_message));
		return;
	}
	while(1){
		sleep(2);
		char commands[1000] = "Commands:\nquery\ndeposit <value>\nwithdraw <value>\nend\n\0";
		char command[10] = "\0";
		write(sock, commands, strlen(commands));
		
		char serv_rcv[1000];
		int len = 0;
		len = read( sock , serv_rcv, 1000);
		strncpy(command, serv_rcv, 10);
		if(len == 0){
			break;
		}
		
		if(strstr(command,"end")){
			allAccounts[i].session_flag = 0;
			char client_message[100] = "service terminated. \n\0";
			printf("%s\n",client_message);
			write(sock, client_message, strlen(client_message));
			break;
		}else if(strstr(command,"deposit ")){
			char *token;
			token = strtok(serv_rcv, " ");
			token = strtok(NULL, " ");

			double value;
			value = atof(token);
			if(value > 0){	
				allAccounts[i].balance += value;
				char success[100];
				sprintf(success, "Deposit successful! Current balance: $%.2f .\0 ",
				allAccounts[i].balance);
				write(sock,success,strlen(success));
			}else{
				char *erro = "Error: Not Valid Amount\n";
				write(sock,erro,strlen(erro));
			}
			
		}else if(strstr(command,"withdraw ")){
			char *token;
			token = strtok(serv_rcv, " ");
			token = strtok(NULL, " ");

			double value;
			value = atof(token);
			if((allAccounts[i].balance - value >= 0) && (value > 0)){
				allAccounts[i].balance -= value;
				char success[100];
				sprintf(success, "Withdrawal successful! Current balance: $%.2f .\0 ",
				allAccounts[i].balance);
				write(sock,success,strlen(success));
			}else{
				char *erro = "Error: Not Valid Amount\n";
				write(sock,erro,strlen(erro));
			}
		}else if(strstr(command, "query")){
			char vals[100];
			sprintf(vals, "Balance: %.2f\n\0", allAccounts[i].balance);
			write(sock,vals,strlen(vals));
		}else{
			char *erro = "Invalid command.\n";
			write(sock,erro,strlen(erro));
		}			
	}
	return;
}

void *connection_handler(void *socket_desc){
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
	int user_index=0;
    int read_size = 0;
    char server_message[1000]  = "received!\n\0" ; 
	char client_message[2000];

    //Receive a message from client
	while(1){
		read_size = recv(sock , client_message , 2000 , 0);
		char command[10] = "\0";
		strncpy(command, client_message, 6);
		printf("%s\n", command);
		if(strstr(command, "create")){
							//removes "create" string
							char *r = strdup(client_message);
							char *tok = r, *end = r;	
							strsep(&end, " ");
							char name[256];
							strncpy(name, end, 255);
							name[strlen(end)-1] = '\0';
							Create(name, sock);
						}
						else if(strstr(command, "serve")){
							char *r = strdup(client_message);
							char *tok = r, *end = r;	
							strsep(&end, " ");
							char name[256];
							strncpy(name, end, 255);
							name[strlen(end)-1] = '\0';
							Serve(name, sock);
						}
						else if(strstr(command, "quit")){
							int i =0;
							for(i=0;i<256;i++){
								if(sock_con[i] == sock){
									sock_con[i] = 0;
									connection_num--;
									break;
								}
							}
							char buffer[10] = "quit \n\0";
							write(sock, buffer, strlen(buffer));
							printf("Client disconnected\n");
							fflush(stdout);
							free(socket_desc);
							return;
						}else{
							char buffer[30] = "Unknown command. \n";
							write(sock, buffer, strlen(buffer));
						}
						
						if(read_size == 0){
							printf("Client disconnected\n");
							fflush(stdout);
							free(socket_desc);
							return;
						}
		
	}
}

// Ctrl-C at keyboard 
void handle_sigint(int sig) { 
	int i = 0;
	char buffer[100] = "server is shutting down. client will exit now. \n\0";
	for(i =0 ; i <= connection_num; i++){
		if(sock_con[i]!=0){
			write(sock_con[i], buffer, strlen(buffer));
		}
	}
	for(i = 0; i < acc_arr_size; i ++){
		allAccounts[i].session_flag = 0;
	}
	printf("Server is shutting down. \n");
	pthread_exit(NULL);
	sem_destroy(&mutex); 
	pthread_mutex_destroy(&lock);
	exit(0);
	return;
} 

int main(int argc, char const *argv[]) 
{ 
	//initialize mutex
	if (pthread_mutex_init(&lock, NULL) != 0) 
    { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 
	//create semaphore
	sem_init(&mutex, 0, 1);
	int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;
	//set up signal
	signal(SIGINT, handle_sigint); 
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1){
        perror("socket failed\n"); 
        exit(EXIT_FAILURE); 
    }

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
	
	if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server))< 0){
        perror("bind failed\n"); 
        exit(EXIT_FAILURE); 
    }
	
	//Listen
    listen(socket_desc , 3);

    printf("Waiting for incoming connections...\n");
    c = sizeof(struct sockaddr_in);
	//install signal alarms
	struct itimerval it_val;
	if (signal(SIGALRM, (void (*)(int)) alarm_handler) == SIG_ERR) {
		perror("Unable to catch SIGALRM");
		exit(1);
	}
	it_val.it_value.tv_sec =     INTERVAL/1000;
	  it_val.it_value.tv_usec =    (INTERVAL*1000) % 1000000;	
	  it_val.it_interval = it_val.it_value;
	  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
		perror("error calling setitimer()");
		exit(1);
	  }
	 while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))){
	
        puts("Connection accepted.\n");
		int i =  0;
		for(i = 0; i<256; i++){
			if(sock_con[i]==0){
				sock_con[i] = client_sock;
				connection_num++;
				break;
			}
		}
        pthread_t sniffer_thread;
        new_sock = (int*) malloc(1);
        *new_sock = client_sock;
        if( pthread_create( &sniffer_thread , NULL ,connection_handler, (void*) new_sock) < 0){
            printf("Error: Thread failed to create\n");
        }

        printf("Handler assigned\n");
    }
    if (client_sock < 0)
    {
        printf("Error: Socket failed to accept\n");
    }
	
	
	sem_destroy(&mutex); 
	pthread_mutex_destroy(&lock);
    return 0;

}


