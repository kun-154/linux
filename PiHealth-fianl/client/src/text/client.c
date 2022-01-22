#include "client.h"
#include <DBG.h>

#define INS 2

char *config = "./PiHealth.conf";
char log_dir[MAX_SIZE] = {0};
char backup_log_dir[MAX_SIZE] = {0};
char PiHealthLog[MAX_SIZE] = {0};
int mem_dyaver = 0;
int master_request;
char master_ip[20] = {0};
int master_port, port;


void *process(void *arg) {
    int client_listen;
    if ((client_listen = socket_create(port)) < 0) {
        DBG("Error create client_listen socket\n");
        exit(1);
    }
    DBG(RED"create listen socket on port %d\n"NONE, port);

    while(1) {
        DBG(RED"accept start\n"NONE);
        master_request = socket_accept(client_listen);
        DBG("Accept connect from Master\n");
        client_process(master_request);
        close(master_request);
    }
    close(client_listen);
    return NULL;
}

void *sys_process(void *arg) {
    long i = (long)arg;

    DBG("%d\n", i);
    while(1) {
        sys_detect((int)i);
        sleep(20);
    }
    return NULL;
}

int sys_detect(int type) {
	char buffer[10240] = {0};
	char buff[512] = {0};
	char buff_tmp[5] = {0};
	FILE *fstream = NULL;
	char run_script[40] = {0};
	char script_name[20] = {0};
	char tmp[20] = {0};
	char logname[50] = {0};
	char times_t[5] = {0};
	int times;
	int socket_common;
	sprintf(tmp, "Scr%d", type);
    DBG("tmp : %s\n", tmp);
	//get_conf_value(config, "SendEveryTimes",times_t);
	get_conf_value(config, tmp, script_name);
    DBG("script_name : %s\n", script_name);
	
	//times = atoi(times_t);
    times = 1;
	if (type == 100) {
		
		sprintf(run_script, "bash ./script/%s %d", script_name, mem_dyaver);
	} else {
		sprintf(run_script, "bash ./script/%s", script_name);
	}
	
	for (int i = 1, lines = 1; i <= times; i++) {
		if (NULL == (fstream = popen(run_script, "r"))) {
			fprintf(stderr, "execute script failed\n");
			return -1;
		}
		if (type == 100) {
			if (NULL != fgets(buff, sizeof(buff), fstream))
			{
				DBG(BLUE"%d : message%d %s"NONE, type, lines++, buff);
				strcat(buffer, buff);
			}
			fgets(buff_tmp, sizeof(buff_tmp), fstream);
			mem_dyaver=atoi(buff_tmp);

		} else {
			while (NULL != fgets(buff, sizeof(buff), fstream)) {
			    DBG(BLUE"%d : message%d %s"NONE, type, lines++, buff);
				strcat(buffer, buff);
			}
		}
		
		if (pclose(fstream) != 0) {
			DBG("This have errors\n");
		}

	}
    DBG("get info done\n");
	generate_logname(type, logname, log_dir);
    DBG("open %s\n", logname);
	FILE* fd = fopen(logname, "a+");
    if (fd == NULL) {
        perror("fopen");
        exit(1);
    }
    DBG("generate_logname: %s\n", logname);
	flock(fd->_fileno, LOCK_EX);
	int ret = fwrite(buffer, 1, strlen(buffer), fd);
    if (ret <= 0) {
        perror("fwrite");
        exit(1);
    }
    DBG("fwrite %d\n", ret);
	fclose(fd);
	check_size(logname, 50, backup_log_dir);
	DBG("File write to log file successfully!\n");
	write_pi_log(PiHealthLog, "Information %d written to log %s successfully.\n", type, logname);

	return 0;
}

int check_log_file(int reqcode, char* log, char* flag) {
	
	char cmd_shell[100] = {0};
	strcpy(log, log_dir);
	switch (reqcode) {
		case 100:
			strcat(log, "/mem.log");
			strcpy(flag, "mem");
			break;
		case 101:
			strcat(log, "/disk.log");
			strcpy(flag, "disk");
			break;
		default:
			break;
	}
	sprintf(cmd_shell, "ls %s/*%s* >/dev/null 2>&1", backup_log_dir, flag);
	int log_t = access(log, R_OK);
	int sys_ret = system(cmd_shell);
	if (log_t == -1 && sys_ret != 0) {
		DBG("file : %s not found\n", log);
		return -1;
	} else if (log_t == 0 && sys_ret != 0) {
		DBG("Log file %s found ,but no ckupfiles\n", log);
		return 1;
	} else if(log_t == -1 && sys_ret == 0) {
		DBG("log file %s not found, but backupfile found \n", log);
		return 2;
	} else if (log_t == 0 && sys_ret == 0) {
		DBG("log file %s and backupfile both found\n", log);
		return 3;
	}
	return 0;
}

void send_file(int sockfd, char* filename){
	FILE* fd = NULL;
	char data[MAX_SIZE];
	size_t num_read;
	fd = fopen(filename, "r");
	DBG("fd open is %d\n", fd);
	flock(fd->_fileno, LOCK_EX);
	if (!fd) {
		DBG("File  %s open error\n", filename);
		fflush(stdout);
		fclose(fd);
		perror("file open");
	} else {
		DBG("File %s open success\n", filename);
		while (1) {
			num_read = fread(data, 1, MAX_SIZE, fd);
            DBG("%s", data);
			if (send(sockfd, data, num_read, 0) < 0) 
				DBG("error in sending file\n");
			if (num_read == 0) {
				break;
			}
		}
		DBG("Sending File %s\n", filename);
		fclose(fd);
	}
}

int client_process(int master_request) {
	int sock_data;
	char log[MAX_SIZE] = {0};
	char flag[10] = {0};
	int ack = 1;
	FILE* fd = NULL;
	char buff[MAX_SIZE] = {0};
	do {
		int ret = recv_response(master_request);
		DBG(RED"Recieved REQUEST code %d\n"NONE, rq);
		if (ret != REQUEST_DATA) {
			DBG("request err!\n");
			send_response(master_request, 444);
			return -1;
		}
        send_response(sockfd, ack);
        if (recv_response(sockfd) <= 0) {
            return -1;
        }
        for (int i = 0; i < INS; ++i) {
            int rq = 100 + i;
		    int check = check_log_file(rq, log, flag);
            DBG(RED"check %s\n"NONE, log);
            
		    if (check < 0){
			    send_response(master_request, rq + 300);
			    DBG(RED"Log file and backup file not found, send RESPONE %d\n "NONE, rq + 300);
			    continue;
		    }
            DBG("check success, send response\n");
		    send_response(master_request, rq + 100);

		    if (recv_response(master_request) == 1) {  //ACK 1
		    
			    DBG("RED%s\n"NONE, "Connection is OK , going to send file");
		    } else {
			    continue;
		    }
		    DBG("check = %d\n", check);
		    if (check == 1) {
			    //send_file(sock_data, log);
                //unlink(log);
			    DBG(RED"File %s sent to Master\n "NONE, log);
		        send_response(master_request, ack);
		        DBG("Sending ack 1 for master close the connect\n");
		        continue;
			}
		}
    }while (0);

	return 0;
}

int client_heart(char *host_t, char *port_t) {
	struct addrinfo hints, *res, *rp;
	DBG("Try to connect with Master %s \n", host_t);
	memset(&hints, 0, sizeof(struct addrinfo));
	char buffer[MAX_SIZE];
	int client_sock_heart;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int s = getaddrinfo(host_t, port_t, &hints, &res);

	if (s != 0 ) {
		DBG("getaddrinfo() error %s\n", gai_strerror(s));

	} 

	for (rp = res; rp != NULL; rp = rp->ai_next) {
		client_sock_heart = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if(client_sock_heart < 0)
			continue;
		if (connect(client_sock_heart, res->ai_addr, res->ai_addrlen) == 0) {
			recv(client_sock_heart, buffer, MAX_SIZE, 0);
			DBG("%s\n", buffer);
			break;
	
		} else {
			//perror("connecting stream socket error ");
			perror(host_t);
			DBG("%s\n", host_t);
			exit(1);
		}
		close(client_sock_heart);
	}
	freeaddrinfo(rp);
	DBG("Connection to %s is OK， Tell Master I'm here!\n", host_t);
	write_pi_log(PiHealthLog, "Connection to %s success\n", host_t);
	return client_sock_heart;	
}



int main(int argc, char *argv[]) {
    pthread_t tid[INS + 1] = {0}, tid_process = 0;
	char port_t[5] = {0}, t_port[6] = {0};
    long temp_i[INS + 1] = {0};
	if (argc != 1) {
		DBG("Usage: ./client\n");
		exit(0);
	}
	get_conf_value(config, "LogDir",log_dir);
	get_conf_value(config, "BackupLogDir",backup_log_dir);
	get_conf_value(config, "PiHealthLog", PiHealthLog);
	get_conf_value(config, "ClientPort",port_t);
	get_conf_value(config, "Master", master_ip);
	get_conf_value(config, "MasterPort", t_port);
    master_port = atoi(t_port);
	port = atoi(port_t);

    pthread_create(&tid_process, NULL, process, NULL);

    /*
    for (long i = 0; i < INS; ++i) {
        temp_i[i] = i + 100; 
        pthread_create(&tid[i], NULL, sys_process, (void *)(temp_i[i]));
    }
    */

    //pthread_join(tid[0], NULL);
    //pthread_join(tid[1], NULL);
    pthread_join(tid_process, NULL);





	return 0;
}
