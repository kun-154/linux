/*************************************************************************
	> File Name: common.h
	> Author: 
	> Mail: 
	> Created Time: Sun 18 Nov 2018 18:33:23 CST
 ************************************************************************/



#ifndef _COMM_H_
#define _COMM_H_
 
  
#include<stdio.h>
#include<sys/shm.h>
#include<errno.h>
#include<sys/types.h>
#include<unistd.h>

#define PATHNAME "."
#define PROJ_ID 0x6666

int creat_shm(size_t size);
int get_shm();
int destroy_shm(int shmid);
#endif
