/*说明：
 * 1：该文档定义TCP协议服务器端基类。
 * 2：执行初始化后，会自动进行连接、接收、发送。
 * 3：当服务器接收到数据后，将数据放到接收消息队列。当需要获取接收数据时，调用Recv直接获取。
 * 4：当需要发送数据时，首先将数据放入发送消息队列。发送线程从发送消息队列获取数据，然后发送。
 * 5：公有变量ErrorMsg用于保存最近一次错误信息。若发生错误，可立刻调用进行查看。
*/
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
class CTcpServer
{
    struct msgbuf
    {
        long msg_type;
        unsigned char data[1024];
    };

public:
    CTcpServer();
    ~CTcpServer();
private:
    int m_nOutTime;         //超时时间。若该时间内没有接收到数据，则断开。
    bool m_bConnectState;   //连接状态
    int m_nTcpServerId;     //socket句柄
    int m_nConnnectId;      //连接句柄
    int m_nSendMsgid;       //发送消息队列
    int m_nRecvMsgid;       //接收消息队列
    struct msqid_ds RecvMsgInfo;    //用于获取接收消息队列信息

    pthread_t m_ConRecvThreadId;    //连接和接收线程
    pthread_t m_SendThreadId;       //发送线程

public:
    int TcpServerInit(int nPort,int nOutTime);
    bool GetConnectState();
    int GetRecvMsgCount();
    int Recv(unsigned char *ucDataBuffer,int nBufferSize);
    int Send(unsigned char *ucSendBuffer,int nBufferSize);

    char ErrorMsg[1024];
private:
    static void TcpServerConRecvThread (void *pParam);
    static void TcpServerSendThread(void *pParam);
};

//#ifdef __cplusplus
//}
//#endif

#endif
