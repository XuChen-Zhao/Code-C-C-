#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "tcp_server.h"
#include <unistd.h>

int main()
{
    CTcpServer my_server;
    if(0 != my_server.TcpServerInit(9595,10))
    {
        printf("%s",my_server.ErrorMsg);
        return -1;
    }

    printf("Start \n");
    unsigned char recv[1024];
    int nRet;

    while(1)
    {
        //printf("%d \n",my_server.GetRecvMsgCount());
        if(1 == my_server.GetConnectState())
        {
            memset(recv,0,1024);
            nRet = my_server.Recv(recv,1024);
            if(0 < nRet)
            {
                my_server.Send(recv,nRet);
            }
        }
        usleep(1000*10);
    }
}
