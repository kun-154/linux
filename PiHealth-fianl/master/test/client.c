/*************************************************************************
	> File Name: client.c
	> Author: 
	> Mail: 
	> Created Time: Sun 18 Nov 2018 18:36:27 CST
 ************************************************************************/

#include"common.h"
 
  
  int main()
  {
        int shmid = get_shm();
        sleep(3);
        char *buf = shmat(shmid,NULL,0);//新的进程挂接到那块共享内存空间，能看到共享内存中的东西。
        
        while (1)
      {
                printf("%s\n",buf);
                sleep(2);
            
      }
        shmdt(buf);
        return 0;

  }
