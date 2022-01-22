#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>		
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>



#define MAX_SIZE 1024
#define MASTER_PORT 8731
/*
* MASTER REQUEST CODE 
* 
* 100 CPU
* 101 MEM
* 102 DISK
* 103 PRO
* 104 SYSINFO
* 105 USERINFO
*
* CLIENT REPLY CODE
* 400 CPU FILE NOT FOUND
* 401 MEM FILE NOT FOUND
* 402 DISK FILE NOT FOUND
* 403 PRO FILE NOT FOUND
* 404 SYSINFO FILE NOT FOUND
* 405 USERINFO
*
* CLIENT REPLY CODE
* 200 CPU file found and ready to transfer
* 201 MEM file found and ready to transfer
* 202 DISK file found and ready to transfer
* 203 PRO file found and ready to transfer
* 204 SYSINFO file found and ready to transfer
* 205 USERINFO
*
*
* 300 CPU normal ok 
* 301 MEM normal ok
* 302 DISK normal ok
* 303 PRO normal ok
* 304 SYSINFO normal ok
* 305 USERINFO
*
*
* 500 CPU file finished
* 501 MEM file finished
* 502 DISK file finished
* 503 PRO file finished
* 504 SYSINFO file finished
* 505 USERINFO
*
*/

int get_conf_value(char *pathname, char* key_name, char *value);

int socket_create(int port);

int socket_accept(int sock_listen);

int socket_connect(int port, char *host);

int recv_data(int sockfd, char* buf, int bufsize);

int send_response(int sockfd, int req);

int recv_response(int sockfd);

int generate_logname(int code, char *logname, char *logdir);

int check_size(char *filename, int size, char *dir);

int write_Pi_log (char *PiHealthLog, const char *format, ...);

#endif
