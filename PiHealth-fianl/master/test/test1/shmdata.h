/*************************************************************************
	> File Name: shmdata.h
	> Author: 
	> Mail: 
	> Created Time: Sun 18 Nov 2018 18:54:07 CST
 ************************************************************************/

#ifndef _SHMDATA_H
#define _SHMDATA_H

#define TEXT_SZ 2048
 
struct shared_use_st
{
    int written; // 作为一个标志，非0：表示可读，0：表示可写
    char text[TEXT_SZ]; // 记录写入 和 读取 的文本
};





#endif
