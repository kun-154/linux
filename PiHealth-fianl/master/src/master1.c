#include "master.h"
#include <DBG.h>

#define INS 2
#define NOTREADY 1010
#define FIN 1019


void * func(void *);
int master_port = 0, client_port = 0, epfd, ack = 1;
char *config = "./pihealth.conf";
char PiHealthLog[MAX_SIZE] = {0};
int queue[INS + 1] = {0};
int start[4] = {0};
int finish[4] = {0};
pthread_mutex_t mutex[INS + 1];
pthread_mutex_t mutex_add = PTHREAD_MUTEX_INITIALIZER;


LinkedList linkedlist[INS + 1];



FILE *log1[INS + 1];

Node insert(LinkedList head, Node *node, int index) {
    Node *p, ret;
    p = &ret;
    ret.next = head;
    while (p->next && index) {
        p = p->next;
        --index;
    }
	if (index == 0) {
        node->next = p->next;
        p->next = node;
    }
	return ret;
}


LinkedList deletenode(LinkedList head, struct sockaddr_in m){
	Node *p,*q,*s;
	p = head;
	q = head->next;
	char ip_m[20] = {0};
	char ip_now[20] = {0};
	sprintf(ip_m, "%s", inet_ntoa(m.sin_addr));
	printf("________________%s________________\n", ip_m);
	while(q){
		sprintf(ip_now, "%s", inet_ntoa(q->addr.sin_addr));
		if(strcmp(ip_m, ip_now) == 0){
            DBG(RED"delete node->ip: %s"NONE, ip_m);
		    s = q;
		    p->next = q->next;
		    q = q->next;
		    free(s); 
            return head;
	    } else {
		    p = p->next;
		    q = q->next;
		}
	}
	return head;
}

int check_online(struct sockaddr_in *check_addr) {
	char check_ip[40] = {0};
	int flag = 1;
	sprintf(check_ip, "%s", inet_ntoa(check_addr->sin_addr));
	for (int i = 0; i < INS; ++i)
	{
		if (check_list(i, check_ip) == -1) {
			flag = 0;
			break;
		}
	}
	return flag;
}

int check(struct sockaddr_in* check_addr) {
    int check_ip[4] = {0};
    ipRange(inet_ntoa(check_addr->sin_addr), check_ip);
    for (int i = 0; i < 4; ++i) {
        if (check_ip[i] >= start[i] && check_ip[i] <= finish[i]) continue;
        return -1;
    }
    return 0;
}
    


int check_list(int num, char *ip) {
	Node *p = linkedlist[num];
	char ip_node[20] = {0};
	while (p) {
		sprintf(ip_node, "%s", inet_ntoa(p->addr.sin_addr));
		printf("***%s\n***%s\n", ip, ip_node);
		if (strcmp(ip, ip_node) == 0)
		{
			return -1;
		}
		p = p->next;
	}
	return 0;
}

void recv_file(int sockfd, char *filename) {
    char buffer[MAX_SIZE] = {0};
    int ret;
    
    FILE *fp = fopen(filename, "a+");
    flock(fp->_fileno, LOCK_EX);
    if (!fp) {
        DBG("file %s open error\n", filename);
        fclose(fp);
        perror("opon");
    }
    while (1) {
        ret = recv(sockfd, buffer, sizeof(buffer), 0);
        if (ret < 0) {
            perror("recv");
            break;
        }else if (ret == 0) {
            DBG("file recv finish\n");
            break;
        }
        DBG("%s", buffer);
        fwrite(buffer, 1, ret, fp);
        fflush(fp);
    }
    DBG("recv file %s done", filename);
    fclose(fp);
}

void request_file(int sockfd, int rq) {
    char filename[128] = {0};
    int ret, ack = 1;
    send_response(sockfd, rq);
    DBG("rq:%d send \n", rq);
    ret = recv_response(sockfd);
    DBG("ret:%d recv \n", ret);
    if (ret == 444) {
        DBG("rq:%d send error\n", rq);
        return ;
    }else if(ret == NOTREADY) {
        DBG("client info not ready\n");
        return ;
    }else if (ret == FIN) {
        DBG("data send fin\n");
        return ;
    }
    send_response(sockfd, ack);
    generate_logname(rq, filename, ".");
    DBG("filename :%s\n", filename);

    recv_file(sockfd, filename);
    DBG("recv file done\n");
    return ;
    
}
/*
void request_all(int sockfd) {
    for (int i = 0; i < INS; ++i) {
        request_file(sockfd, i + 100);
    }
}
*/

int request_all(LinkedList p) {
    char buffer[512] = {0};
    int ret;
    for (int i = 0; i < INS; ++i) {
        int sockfd = socket_connect(ntohs(p->addr.sin_port), inet_ntoa(p->addr.sin_addr));
        if( sockfd < 0) {
            return -1;
        } else {
            DBG(GREEN"connected to %s:%d\n"NONE, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
            ret = recv(sockfd, buffer, sizeof(buffer), 0);
            DBG(GREEN"recv ack from client : %s\n"NONE, buffer);

            request_file(sockfd, i + 100); // 从客户端请求运行状态文件
            DBG("request_file done\n");
            close(sockfd);
        }
    }
    return 0;
}

void output(LinkedList head, int num) {
	Node *p = head;
    char buffer[MAX_SIZE] = {0};
    int ret;
	char logfile[20];
    sprintf(logfile, "%d.log", num);
    log1[num] = fopen(logfile, "w");
    if (log1[num] == NULL) {
        perror(YELLOW"fopen\n"NONE);
    }

    fflush(stdout);
    while (p) {
    	if (p->addr.sin_addr.s_addr  == ntohl(INADDR_ANY)) {

        	DBG("%dth ^^^^^^^^^^^^^^^^^^^in 0.0.0.0\n", num);
        	if (p && p->next) {
                p = p->next;
                continue;
            }else {
                goto end;
            }
        }
        //DBG("%dth----%s", num, "In  output 1th\n");
        fprintf(log1[num], "%s:%d\n", inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
        //DBG("connect to %s:%d", inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
        /*
        int sockfd = socket_connect(ntohs(p->addr.sin_port), inet_ntoa(p->addr.sin_addr));
        if( sockfd < 0) {
        DBG(RED"connect to %s:%d error, deletenode"NONE, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
        	pthread_mutex_lock(&mutex[num]);
        	deletenode(head, p->addr);
        	queue[num]--;
        	DBG("We  have %d  left!--------queue\n", queue[num]);
        	pthread_mutex_unlock(&mutex[num]);
        } else {
            DBG(GREEN"connected to %s:%d\n"NONE, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
            ret = recv(sockfd, buffer, sizeof(buffer), 0);
            DBG(GREEN"recv ack from client : %s\n"NONE, buffer);

            request_all(sockfd); // 从客户端请求运行状态文件
            DBG("request_all done\n");
            close(sockfd);
        }
        */
        ret = request_all(p);
        if (ret == -1) {
            DBG(RED"connect to %s:%d error, deletenode"NONE, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
        	pthread_mutex_lock(&mutex[num]);
        	deletenode(head, p->addr);
        	queue[num]--;
        	DBG("We  have %d  left!--------queue\n", queue[num]);
        	pthread_mutex_unlock(&mutex[num]);
        }
        
        if (p && p->next) p = p->next;
    }
    end:
    //DBG(YELLOW"%dth output out\n"NONE, num);
	fclose(log1[num]);
    return ;
}

void clear(LinkedList head) {
	Node *p, *q;
    p = head;
    while (p) {
        q = p->next;
        free(p);
        p = q;
    }
    return;
}

int find_min(int N, int *arr) {
	int  *min = arr;
	int ans = 0;
	for (int i = 0; i < N; ++i) {
		if(*(arr + i) < *min){
			min = arr + i;
			ans = i;
		}
	}
	return ans;
}

void * print(void *arg) {
	int *i = (int *)arg;
	while (1) {
		sleep(2);
		output(linkedlist[*i], *i);
	}
	return NULL;
}

/*
void *addnode(void *argv) {
	struct sockaddr_in *client_addr, server_addr;
	char buffer[50];
	struct mypara *para = (struct mypara *) argv;
	int fd = para->fd;
	client_addr = &(para->client_addr);
	socklen_t len = sizeof(server_addr);
	getsockname(fd, (struct sockaddr *) &server_addr, &len);
	sprintf(buffer, "%s:%d --> You have connected to Server!", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
	send(fd, buffer, strlen(buffer), 0);
	DBG("%s:%d Login Server!\n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
	close(fd);
	client_addr->sin_port = htons(client_port);
	Node *p , ret;
	p = (Node*) malloc(sizeof(Node));
	p->addr = *client_addr;
	p->next = NULL;
	int sub = find_min(INS, queue);
	pthread_mutex_lock(&mutex[sub]);
	ret = insert(linkedlist[sub], p, queue[sub]);
	queue[sub]++;
	linkedlist[sub] = ret.next;
	pthread_mutex_unlock(&mutex[sub]);
	printf("--------------------->--------->---------->\n");
	pthread_mutex_unlock(&mutex_add);
	return NULL;
}
*/

int addnode(struct mypara* para) {
	pthread_mutex_lock(&mutex_add);
	struct sockaddr_in *client_addr, server_addr;
	char buffer[50];
	int fd = para->fd;
	client_addr = &(para->client_addr);
	socklen_t len = sizeof(server_addr);
	getsockname(fd, (struct sockaddr *)&server_addr, &len);
	sprintf(buffer, "%s:%d --> You have connected to Server!", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
	send(fd, buffer, strlen(buffer), 0);
    // 
	DBG("%s:%d Login Server!\n", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
	close(fd);
	client_addr->sin_port = htons(client_port);
	Node *p, ret;
	p = (Node*)malloc(sizeof(Node));
	p->addr = *client_addr;
	p->next = NULL;
	int sub = find_min(INS, queue);
	pthread_mutex_lock(&mutex[sub]);
	ret = insert(linkedlist[sub], p, queue[sub]);
	queue[sub]++;
	linkedlist[sub] = ret.next;
	pthread_mutex_unlock(&mutex[sub]);
	pthread_mutex_unlock(&mutex_add);
    DBG(GREEN"-------------------add node done--------------\n"NONE);
	return 0;
}

int ipRange (char *ip, int *ipadr) {
	if (ip == NULL) return -1;
	char temp[4];
	int count = 0;
	while (1) {
		int index = 0;
		while (*ip != '\0' && *ip != '.' && count < 4) {
			temp[index++] = *ip;
			ip++;
		}
		if (index  == 4) return -1;
		temp[index]='\0';
		ipadr[count] = atoi(temp);
		if (ipadr[count] > 255 || ipadr[count] < 0 ) return -1;
		count++;

		if (*ip == '\0') {
			if (count != 4) return -1;
		} else {
			ip++;
		}
	}
	return 0;
}

void *echo_heart(void* arg) {
    pthread_detach(pthread_self());
    int *t = (int*)arg;
    int ret, socket = *t;

    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    socklen_t len = sizeof(client_addr); 
    getsockname(socket, (struct sockaddr *)&client_addr, &len);
    if (check_online(&client_addr) == 0) {
        ret = send_response(socket, HEART);
        if (ret == -1) {
            DBG(RED"send_response error\n"NONE);
            perror("send");
        }
    }else {
        ret = send_response(socket, -1);
        if (ret == -1) {
            DBG(RED"send_response error\n"NONE);
            perror("send");
        }
    }
    close(socket);
    return NULL;
}

void *login(void *arg) {
    pthread_detach(pthread_self());
    int *t = (int*)arg;
    int ret, socket = *t;
    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    socklen_t len = sizeof(client_addr); 
    getsockname(socket, (struct sockaddr *)&client_addr, &len);
    if (check(&client_addr) == 0) {
        struct mypara newClient;
        newClient.fd = socket;
        newClient.client_addr = client_addr;
        addnode(&newClient);
    }
    close(socket);
    return NULL;
    
}

int main() {

	int master_listen;
	char prename[15] = {0};
	char start_ip[20] = {0};
	char finish_ip[20] = {0};
	char hostname[20] = {0};
	char temp_client[5] = {0};
	char temp_master[5] = {0};
	get_conf_value(config, "master_port", temp_master);
	get_conf_value(config, "client_port", temp_client);
	master_port = atoi(temp_master);
	client_port = atoi(temp_client);
	//get_conf_value(config, "PiHealthLog", PiHealthLog);
	get_conf_value(config, "From", start_ip);
	get_conf_value(config, "To", finish_ip);

	pthread_t master_t;
	pthread_t t[INS + 1];
	fflush(stdout);

	struct sockaddr_in initaddr;
	initaddr.sin_family = AF_INET;
	initaddr.sin_port = htons(client_port);
	

	ipRange(start_ip, start);
	ipRange(finish_ip, finish);

	/*
	初始化头结点
	 */
	
	DBG("%s", "初始化头结点      start!\n");
	for (int i = 0; i < INS; ++i)  {
		initaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
		Node *p, ret;
		p = (Node *)malloc(sizeof(Node));
		p->addr = initaddr;
		p->next = NULL;
		pthread_mutex_lock(&mutex[i]);
		ret = insert(linkedlist[i], p, queue[i]);
		queue[i]++;
    	linkedlist[i] = ret.next;
    	pthread_mutex_unlock(&mutex[i]);
	}
	DBG("%s", "初始化头结点      success!\n");
	/*
	初始化线程锁
	 */
	pthread_mutexattr_t attr;
	

	if(pthread_mutexattr_init(&attr) != 0) {
		perror("Error Init mutexattr");
	}
	for (int i = 0; i < INS; ++i)
	{
		if(pthread_mutex_init(&mutex[i], &attr) != 0) {
			perror("Error Init mutex");
		}
	}


	/*
	初始化队列
	 */
	DBG("%s", "初始化队列     开始！\n");
	for (int i = start[3]; i <= finish[3]; ++i) {
		//printf("_______________________________________________%s.%d", prename, i);
		sprintf(hostname, "%d.%d.%d.%d", start[0], start[1], start[2], i);
		//printf("*****************************%s\n\n", hostname);
		initaddr.sin_addr.s_addr = inet_addr(hostname);
		int sub = find_min(INS, queue);
		Node *p, ret;
		p = (Node *)malloc(sizeof(Node));
		p->addr = initaddr;
		p->next = NULL;
		pthread_mutex_lock(&mutex[sub]);
		ret = insert(linkedlist[sub], p, queue[sub]);
		queue[sub]++;
     	linkedlist[sub] = ret.next;
		pthread_mutex_unlock(&mutex[sub]);
	}
	DBG("%s", "初始化队列     success！\n");

	

	/*
	创建INS个子线程用来遍历处理各自线程对应的队列
 	*/	
    int temp_t[INS] = {0};

	for (int i = 0; i < INS; ++i)
	{	
        /*
        for (int j = 0; j < queue[i]; ++j) {
            DBG(GREEN"linkedlist[%d][%d] ip : %s\n"NONE, i, j, inet_ntoa(p->addr.sin_addr));
            p = p->next;
        }
        */
        temp_t[i] = i;
		if (pthread_create(&t[i], NULL, print, (void*)&temp_t[i])) {
			printf("error!\n");
			exit(1);
		}
		DBG("Current pthread id =%ld deal list %d\n", t[i], i);
	}

	DBG("%s","主线程等待中.....\n");
	fflush(stdout);
    

    int ret;
    master_listen = socket_create_nonblock(master_port); // 监听来自客户端的请求
    struct timeval tm;
    tm.tv_sec = 10;
    tm.tv_usec = 0;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(master_listen, &set);

    while (1) {
        struct timeval t_tm = tm;
        pthread_t temp;
        fd_set t_set = set;
        ret = select(master_listen + 1, &t_set, NULL, NULL, &t_tm);
        if (ret < 0) {
            if (errno == EAGAIN) {
                DBG(YELLOW"EAGAIN\n");
                continue;
            }
            DBG(RED"----------select error %s\n"NONE, strerror(errno));
            perror("select");
            exit(1);
        }else if (ret == 0) {
            DBG(YELLOW"ret = 0\n");
            continue;
        }

        int socket = listen_accept(master_listen);
        ret = recv_response(socket);
        if (ret == HEART) {
            pthread_create(&temp, NULL, echo_heart, (void *)&socket);
        }else if (ret == LOGIN) {
            pthread_create(&temp, NULL, login, (void *)&socket);
        }else {
            close(socket);
        }
    }




	pthread_join(master_t, NULL);
	pthread_join(t[0],NULL);
	pthread_join(t[1],NULL);
	pthread_join(t[2],NULL);
	pthread_join(t[3],NULL);
	pthread_join(t[4],NULL);
    
    for (int i = 0; i < INS; ++i) {
        clear(linkedlist[i]);
        DBG("clear LinkedList[%d]\n", i);
    }

    //if ((epfd = epoll_create(128)) < 0) {
      //  perror("epoll_create");
        //exit(1);
    //}
    //struct epoll_event events[MAX_SIZE], ev;

    /*
    ev.data.fd = master_listen, ev.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, master_listen, &ev) < 0) {
        perror("epoll_ctl");
        exit(1);
    }
    */
    /*
    while (1) {
        int nfd = epoll_wait(epfd, events, 1024, -1);
        if (nfd <= 0) {
            perror("epoll_wait");
            exit(1);
        }
        for (int i = 0; i < nfd; ++i) {
            int fd = events[i].data.fd, listen_response, sockfd_t;
            DBG(BLUE"epoll_wait nfd : %d"NONE, nfd);
            if (fd == master_listen) {
                struct sockaddr_in cli_addr;
                socklen_t addr_len = sizeof(cli_addr);
                sockfd_t = accept(fd, (struct sockaddr *)&cli_addr, &addr_len);
                if (sockfd_t < 0) {
                    perror("accept");
                    exit(1);
                }
                listen_response = recv_response(sockfd_t);
                if (listen_response == HEART) {
                    if (check(cli_addr) == 0) {
                        if (send_response(sockfd_t, HEART) == -1) {
                            continue;
                        }
                    }else {
                        if (send_response(sockfd_t, 0) == -1) {
                            continue;
                        }
                    }
                    close(sockfd_t);
                }else if (listen_response == LOGIN){
                    if (check(cli_addr) == 1) {
                        struct mypara newClient;
                        newClient.fd = sockfd_t;
                        newClient.client_addr = cli_addr;
                        addnode((void *)&newClient);
                    }else {
                        DBG("client repeat login\n");
                        close(sockfd_t);
                    }
                }
            }
        }
    }
    */

	return 0;
}



