#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <pthread.h> 
#include <unistd.h>
#include <signal.h>
#include "bankingClient.h"

   
struct sockaddr_in address; 
int sock = 0, valread;
int server_connect_flag = 0;  
int service_flag = 0;
struct sockaddr_in serv_addr;
pthread_t user_tid;

void alarm_handler(int sig){
	if(server_connect_flag == 0){
		printf("Could not connect to server!\n");
		exit(0);
	}else{
		return ;
	}
	signal(sig, alarm_handler);
}	
   
void *serverResponse(void *vargp){ 
   int quit_flag = 0;
   while(quit_flag == 0){
	   int serv_resp_len = 0;
	   char serv_resp[1000] = "\0";
	   serv_resp_len = read(sock, serv_resp, 999);
	   serv_resp[serv_resp_len] = '\0';
	   if(strstr(serv_resp, "service in session")){
		   service_flag = 1;
		    printf("%s\n", serv_resp);
	   }
	   else if(strstr(serv_resp, "service terminated")){
		   service_flag = 0;
		    printf("%s\n", serv_resp);
	   }
	   else if(strstr(serv_resp,"server is shutting down.")){
			pthread_cancel(user_tid);
			printf("%s\nexiting client.\n", serv_resp);
			quit_flag = -1 ;
			return;
	   }
	   else if(strstr(serv_resp, "quit")){
		   if(service_flag == 1){
			   printf("please end service before quitting \n"); 
		   }else{
			   printf("Disconnected From Server.\n");
				quit_flag = -1 ;
				return;
		   }
	   }else{
		   printf("%s\n", serv_resp);
	   }
   } 
}    
 

 void *userResponse(void *vargp){ 

   while(1){
		sleep(2);
		char client_msg[1000];
		if(service_flag == 0){
			 printf("\nCommands: \ncreate <account name> \nserve <account name> \nquit\n\n");
		}
		//gets user input
		fgets(client_msg, 1000, stdin);
		sleep(2);
		write(sock, client_msg, 1000);
		//checks client message
		if((strstr(client_msg, "quit") && service_flag == 0)){
			printf("exiting client \n");
			return;
		}
			
   } 
}    

int main(int argc, char const *argv[]) { 
	
	//sets up signal handler for alarm
	signal(SIGALRM, alarm_handler);
	
	//resp for responses
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    memset(&serv_addr, '0', sizeof(serv_addr)); 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
   
   alarm(60);
    while(1){
		if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){ 
			printf("Connection Failed, Retrying! \n"); 
			sleep(3);
		}else{
			server_connect_flag = 1;
			printf("Connected to server. \n");
			break;
		}
	}
	
	pthread_t serv_tid;
	pthread_create(&serv_tid, NULL, serverResponse, NULL); 
	pthread_create(&user_tid, NULL, userResponse, NULL); 
	pthread_join(serv_tid, NULL); 
	pthread_join(user_tid, NULL); 

    return 0;
} 
