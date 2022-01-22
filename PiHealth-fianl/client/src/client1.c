#include "client.h"
#include <DBG.h>

#define INS 2
#define NOTREADY 1010
#define FIN 1019

char *config = "./PiHealth.conf";  // conf文件
char log_dir[MAX_SIZE] = {0};   // 日志文件
char backup_log_dir[MAX_SIZE] = {0}; // 数据打包文件 的目录
char PiHealthLog[MAX_SIZE] = {0}; // 监控系统的日志文件名
int mem_dyaver = 0;  //内存的动态值
int master_request;  // 
char master_ip[20] = {0}; 
int master_port, client_port;


void *process(void *arg) {
    int client_listen;
    if ((client_listen = socket_create(client_port)) < 0) {
        DBG("Error create client_listen socket\n");
        exit(1);
    }
    DBG(RED"create listen socket on port %d\n"NONE, client_port);

    while(1) {
        DBG(RED"accept start\n"NONE);
        master_request = socket_accept(client_listen);
        if (master_request < 0) {
            DBG(RED"socket_accept failed\n"NONE);
            continue;
        }
        DBG("Accept connect from Master\n");
        client_process(master_request);
        close(master_request);
    }
    close(client_listen);
    return NULL;
}

void *sys_process(void *arg) {
    long i = (long)arg;

    DBG(YELLOW"sys_process %d start work!!\n"NONE, i);
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
    //DBG("tmp : %s\n", tmp);
	//get_conf_value(config, "SendEveryTimes",times_t);
	get_conf_value(config, tmp, script_name);
    DBG(GREEN"get script_name : %s from conf... \n"NONE, script_name);
	
	//times = atoi(times_t);
    times = 1;
	if (type == 100) {
		
		sprintf(run_script, "bash ./script/%s %d", script_name, mem_dyaver);
	} else {
		sprintf(run_script, "bash ./script/%s", script_name);
	}
	
	for (int i = 1, lines = 1; i <= times; i++) {
		if (NULL == (fstream = popen(run_script, "r"))) {
			DBG(RED"\n\nexecute script failed!!!!\n\n"NONE);
            perror("popen");
			return -1;
		}
		if (type == 100) {
			if (NULL != fgets(buff, sizeof(buff), fstream))
			{
				DBG(BLUE"%d : message%d %s"NONE, type, lines++, buff);
				strcat(buffer, buff);
			}
			fgets(buff_tmp, sizeof(buff_tmp), fstream);
			mem_dyaver = atoi(buff_tmp); // 更新 动态 内存值
		} else {
			while (NULL != fgets(buff, sizeof(buff), fstream)) {
                DBG(GREEN"%d line: %d ... message:\t  %s"NONE, type, lines++, buff);
				strcat(buffer, buff);
			}
		}
		
		if (pclose(fstream) != 0) {
			DBG("This have errors\n");
		}

	}
    DBG(RED"\n----------get info done---------------\n"NONE);
	generate_logname(type, logname, log_dir); // 获取数据文件存放 目录 与 文件名 组合
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
	check_size(logname, 5, backup_log_dir);
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
    if (fd == NULL) {
        DBG(RED"filename is not ready\n"NONE);
        fclose(fd);
        return ;
    }
    DBG("filename %s : fd open is %d\n", filename, fd);
	flock(fd->_fileno, LOCK_EX);
	DBG("File %s open success\n", filename);
	while (1) {
		num_read = fread(data, 1, MAX_SIZE, fd);
        DBG("%s", data);
        if (num_read == 0) {
            break;
        } if (send(sockfd, data, num_read, 0) < 0){ 
			DBG("error in sending file\n");
		}
	}
	DBG("Sending File %s\n", filename);
	fclose(fd);
}

int client_process(int master_request) {
	int sock_data;
	char log[512] = {0};
	char flag[10] = {0};
	int ack = 1, ret;
	FILE* fd = NULL;
	char buff[MAX_SIZE] = {0};
	do {
		int rq = recv_response(master_request);
		DBG(RED"Recieved REQUEST code %d\n"NONE, rq);
		if (rq < 100 || rq > 100 + INS) {
			DBG("request err!\n");
			send_response(master_request, 444);
			break;
        }else if (rq == FIN) {
            if (send_response(master_request, FIN) == -1) {
                perror("send_response");
                DBG("send");
                break;
            }
            DBG("request fin\n");
            break;
        }
		int check = check_log_file(rq, log, flag);
        DBG(RED"check %s\n"NONE, log);
            
		if (check < 0){
			DBG(RED"Log file and backup file not found, send RESPONE %d\n "NONE, rq + 300);
            send_response(master_request, NOTREADY);
            break;
		}
        if (send_response(master_request, ack) < 0) {
            DBG("send response");
            break;
        }
        if (recv_response(master_request) < 0) {
            DBG(RED"recv_response error \n"NONE);
            break;
        }
        DBG(GREEN"check success, send response\n"NONE);

		send_file(master_request, log);
		DBG(RED"File %s sent to Master\n "NONE, log);
        unlink(log);

    }while (0);

    DBG("client_process fin\n");
    close(master_request);

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

int relogin() {
    return 1;
}

void heart_beat(int signum) {
   int ack = 1, ret; 
    int sockfd = socket_connect(master_port, master_ip);
    if (sockfd <= 0) {
        DBG(RED"\nsocket_connect failed\n"NONE);
        perror("connect");
        return ;
    }
    DBG(GREEN"\nsocket_connect success\n"NONE);
    if (send_response(sockfd, HEART) == -1) {
        DBG(RED"\nsend_response\n"NONE);
        perror("send response");
        return ;
    }
    DBG(GREEN"\nsend_response success\n"NONE);
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (select(sockfd + 1, &rfds, NULL, NULL, &tv) <= 0) {
        close(sockfd);
        DBG(YELLOW"time out from hear_beat\n"NONE);
    }
    if ((ret = recv_response(sockfd) == -1)) {
        perror("recv_response");
    }
    DBG(GREEN"recv_response from master %d\n"NONE, ret);
    if (ret == HEART) {
        DBG(GREEN"connect status: %d good\n"NONE, ret);
    }else if (ret == 0) {
        DBG(RED"client loss server connect, try relogin...\n"NONE);
        if (relogin()) {
            DBG("relogin to server success\n");
        }else {
            DBG(RED"failed to relogin to server\n"NONE);
        }
    }

    return ;
}

int main(int argc, char *argv[]) {
    pthread_t tid[INS + 1] = {0}, tid_process = 0; // 线程 id 
	char _client_port[5] = {0}, _master_port[6] = {0};
    long temp_i[INS + 1] = {0};

	get_conf_value(config, "LogDir",log_dir);
	get_conf_value(config, "BackupLogDir",backup_log_dir);
	get_conf_value(config, "PiHealthLog", PiHealthLog);

	get_conf_value(config, "ClientPort", _client_port);
	get_conf_value(config, "Master", master_ip);
	get_conf_value(config, "MasterPort", _master_port);
	client_port = atoi(_client_port);
    master_port = atoi(_master_port);

    signal(SIGALRM, heart_beat);
    
    struct itimerval itimer;
    itimer.it_interval.tv_sec = 3;
    itimer.it_interval.tv_usec = 0;
    itimer.it_value.tv_sec = 10;
    itimer.it_value.tv_usec = 0;

    setitimer(ITIMER_REAL, &itimer, NULL); // 设置闹钟 定时 激活 心跳 信号


    pthread_create(&tid_process, NULL, process, NULL);
    for (long i = 1; i < INS; ++i) { // 开启 数据收集模块 线程
        temp_i[i] = i + 100; 
        pthread_create(&tid[i], NULL, sys_process, (void *)(temp_i[i]));
    }

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid_process, NULL);





	return 0;
}
