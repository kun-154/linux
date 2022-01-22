#ifndef CLIENT_H
#define CLIENT_H
#define _DEBUG

#include "common.h"
#include "head.h"

void send_file(int sockfd, char* filename);

void *sys_process(void *arg);

int client_process(int master_request); 
//int client_recv_req(int master_request, char* req, char* arg);
int client_start_data_conn(int master_request);

int check_log_file(int reqcode, char *log, char *flag);

int sys_detect(int type);

int client_heart(char *host_t, char *port_t);

#endif
