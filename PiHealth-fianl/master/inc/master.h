/*************************************************************************
	> File Name: master.h
	> Author: 
	> Mail: 
	> Created Time: 2021年12月25日 星期六 13时51分46秒
 ************************************************************************/

#ifndef _MASTER_H
#define _MASTER_H
#include "head.h"
#include "common.h"

struct mypara{
    struct sockaddr_in client_addr;
    int fd;
};

typedef struct Node {
    struct sockaddr_in addr;
    struct Node *next;
}Node, *LinkedList;

void *echo_heart(void *arg);

void *login(void *arg);

int check_list(int num, char *ip);

Node insert(LinkedList head, Node *node, int index);

LinkedList deletenode(LinkedList head, struct sockaddr_in m);

int check(struct sockaddr_in* check_addr);

int check_online(struct sockaddr_in *check_addr);

void output(LinkedList head, int num);

void clear(LinkedList head);

int find_mid(int N, int *arr);

void *print(void *arg);

int addnode(struct mypara *argv);

int ipRange(char *ip, int *ipadr);

#endif
