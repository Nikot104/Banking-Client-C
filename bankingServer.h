#define PORT 8338
#define INTERVAL 15000

typedef struct Accounts{
	char *name;
	double balance;
	int session_flag;
} Account;

int Create(char* name, int sock);

void alarm_handler(int sig);

void Serve(char* name, int sock);

void *connection_handler(void *socket_desc);

void handle_sigint(int sig);