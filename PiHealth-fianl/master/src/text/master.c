#include "master.h"
#include <DBG.h>

#define INS 2


void * func(void *);
int master_port = 0, client_port = 0, epfd;
char *config = "./pihealthd.conf";
char PiHealthLog[MAX_SIZE] = {0};
int queue[INS + 1] = {0};
pthread_mutex_t mutex[INS + 1];
pthread_mutex_t mutex_add = PTHREAD_MUTEX_INITIALIZER;


LinkedList linkedlist[INS + 1];



FILE *log1[INS + 1];

Node insert(LinkedList head, Node *node, int index) {
    Node *p, ret;
    p = &ret;
    //ret.addr = 0;
    ret.next = head;
    while (p->next && index) {
        p = p->next;
        --index;
    }
	if (index == 0) {
        node->next = p->next;
        p->next = node;
        //ret.addr = 1;
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
		p->next = q->next;
		s = q;
		q = q->next;
		free(s); 
	} else {
		p = p->next;
		q = q->next;
		}
	}
	return head;
}

int check(struct sockaddr_in check_addr) {
	char check_ip[20] = {0};
	int flag = 1;
	sprintf(check_ip, "%s", inet_ntoa(check_addr.sin_addr));
	for (int i = 0; i < INS; ++i)
	{
		if (check_list(i, check_ip) == -1) {
			flag = 0;
			break;
		}
	}

	return flag;
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

void output(LinkedList head, int num) {
	Node *p = head;
    int ret, ack = 1, t;
	char logfile[20];
    sprintf(logfile, "%d.log", num);
    log1[num] = fopen(logfile, "w");
    fflush(stdout);
    while (p) {

    	if (p->addr.sin_addr.s_addr  == ntohl(INADDR_ANY)) {

        	//DBG("%dth ^^^^^^^^^^^^^^^^^^^in 0.0.0.0\n", num);
        	p = p->next;
        	continue;
        }
        //DBG("%dth----%s", num, "In  output 1th\n");
        fprintf(log1[num], "%s:%d\n", inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
        //DBG("connect to %s:%d", inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
        int sockfd = socket_connect(ntohs(p->addr.sin_port), inet_ntoa(p->addr.sin_addr));
        if( sockfd < 0) {
        DBG(RED"connect to %s:%d error, deletenode"NONE, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
        	pthread_mutex_lock(&mutex[num]);
        	deletenode(head, p->addr);
        	queue[num]--;
        	printf("We  have %d  left!--------queue\n", queue[num]);
        	p = p->next;
        	pthread_mutex_unlock(&mutex[num]);

        } else {
            DBG(GREEN"connected to %s:%d"NONE, inet_ntoa(p->addr.sin_addr), ntohs(p->addr.sin_port));
            if (send_response(sockfd, REQUEST_DATA) < 0) {
                perror("send_response");
                exit(1);
            }
            if (recv_response(sockfd) != ack) {
                close(sockfd);
                p = p->next;
            }
            if (send_response(sockfd, ack) < 0) {
                perror("send_response");
                exit(1);
            }
            DBG(RED"resopnse sended :%d\n"NONE, REQUEST_DATA);
            for (int i = 0; i < INS; ++i) {
                int rq = recv_response(sockfd);
                if (rq == i + 200) {
                    struct epoll_event ev;
                    ev.data.fd = sockfd, ev.events = EPOLLIN;
                    DBG(GREEN"epoll_ctl_add\n"NONE);
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
                        perror("epoll_ctl");
                        exit(1);
                    }
                }else if (t == 444) {
                    DBG("client info not ready\n");
                }else {
                    DBG("recv client response %d\n", t);
                }

            }
        	p = p->next;
        }

    }
	fclose(log1[num]);
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

void *print(void *arg) {
	int i ;
	memcpy(&i, (void *)&arg, 4); 
	while (1) {
		//DBG("Thread in %d !\n", i);
		sleep(2);
		output(linkedlist[i], i);
	}
	return NULL;
}


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

int main() {

	int master_listen, ack = 1;
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
	
	int start[4] = {0};
	int finish[4] = {0};

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
		pthread_mutex_unlock(&mutex[sub]);
     	linkedlist[sub] = ret.next;
	}
	DBG("%s", "初始化队列     success！\n");

	
	long temp_i[INS + 1]= {0};

	/*
	创建INS个子线程用来遍历处理各自线程对应的队列
 	*/	

	for (int i = 0; i < INS; ++i)
	{	
		temp_i[i] = i;
		if (pthread_create(&t[i], NULL, print, (void*)temp_i[i])) {
			printf("error!\n");
			exit(1);
		}
		DBG("Current pthread id =%ld \n", t[i]);
	}

	DBG("%s","主线程等待中.....\n");
	fflush(stdout);
    
    if ((epfd = epoll_create(128)) < 0) {
        perror("epoll_create");
        exit(1);
    }
    struct epoll_event events[MAX_SIZE], ev;
    master_listen = socket_create(master_port);
    ev.data.fd = master_listen, ev.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, master_listen, &ev) < 0) {
        perror("epoll_ctl");
        exit(1);
    }
    int ret;
    while (1) {
        int nfd = epoll_wait(epfd, events, 1024, -1); //  阻塞 epoll_wait -1
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
                    int haert_response = send_response(fd, HEART);
                    close(sockfd_t);
                }else if (listen_response == LOGIN){
                    if (check(cli_addr)) {
                        struct mypara newClient;
                        newClient.fd = sockfd_t;
                        newClient.client_addr = cli_addr;
                        addnode((void *)&newClient);
                    }
                }
            }else {
                DBG(RED"client start send file data\n"NONE);
                for (int i = 0; i < INS; ++i) {
                    listen_response = recv_response(fd);
                    DBG(BLUE"listen_response : %d\n"NONE, listen_response);
                    if (listen_response == i + 400) {
                        DBG("client info not ready\n");
                        continue;
                    }
                    send_response(fd, ack);
                    char filename[128] = {0};
                    generate_logname(i + 100, filename, ".");
                    DBG(RED"recv file %d from client\n"NONE, filename);
                    /*
                    FILE *fp = fopen(filename, "a+");
                    char buffer_data[4096] = {0};
                    while((ret = recv(fd, buffer_data, sizeof(buffer_data), 0)) > 0) {
                        DBG("%s", buffer_data);
                        fwrite(buffer_data, 1, ret, fp);
                        fflush(fp);
                    }
                    fclose(fp);
                    */
                    int listen_response = recv_response(fd);
                    if (listen_response == 1) {
                        DBG("success recv file: %s\n", filename);
                    }
                }
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            }
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

	return 0;
}



