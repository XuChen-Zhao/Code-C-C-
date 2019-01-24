#include "tcp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

CTcpServer::CTcpServer()
{
    m_nOutTime = -1;
    m_bConnectState = 0;
    m_nTcpServerId = -1;
    m_nConnnectId = -1;
}

CTcpServer::~CTcpServer()
{

}

/*======================================================================//
函数功能：该函数有对tcp服务器类进行初始化。包括初始化发送队列和接收队列，socket初始化，
            相关线程初始化。
输入参数：
    int nPort                       端口号
    int nOutTime                    超时时间
输出参数：
    无
返回：
    int                             0：正确  <0:错误
/=======================================================================*/
int CTcpServer::TcpServerInit(int nPort,int nOutTime)
{
    if((0 >= nPort) || (0 >= nOutTime))
    {
        sprintf(ErrorMsg,"%s","端口号或超时时间设置错误");
        return -1;
    }
    //发送消息队列初始化
    key_t key;
    key = ftok(".",'b');
    m_nSendMsgid = msgget(key,IPC_CREAT|IPC_EXCL|0666);
    if((-1 == m_nSendMsgid) && (errno == EEXIST))
    {
        key=ftok(".",'b');
        m_nSendMsgid = msgget(key, IPC_CREAT|0666);
    }
    if(0 > m_nSendMsgid)
    {
        sprintf(ErrorMsg,"%s","创建发送队列失败");
        return -2;
    }
    //接收消息队列初始化
    key = ftok(".",'a');
    m_nRecvMsgid = msgget(key,IPC_CREAT|IPC_EXCL|0666);
    if((-1 == m_nRecvMsgid) && (errno == EEXIST))
    {
        key=ftok(".",'a');
        m_nRecvMsgid = msgget(key, IPC_CREAT|0666);
    }
    if(0 > m_nRecvMsgid)
    {
        sprintf(ErrorMsg,"%s","创建接收队列失败");
        return -3;
    }

    //服务器初始化
    m_nTcpServerId = socket(AF_INET,SOCK_STREAM, 0);
    if(0 >= m_nTcpServerId)
    {
        sprintf(ErrorMsg,"%s","创建socket失败");
        return -4;
    }

    struct timeval timeout={nOutTime,0};
    if(0 != setsockopt(m_nTcpServerId,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout)))
    {
        sprintf(ErrorMsg,"%s","设置超时时间失败");
        return -5;
    }

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(nPort);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(0 != bind(m_nTcpServerId,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr)))
    {
        sprintf(ErrorMsg,"%s","bind失败");
        return -6;
    }

    if(0 != listen(m_nTcpServerId,20))
    {
        sprintf(ErrorMsg,"%s","listen失败");
        return -7;
    }

    //速通连接和接收线程
    if(0 != pthread_create(&m_ConRecvThreadId,NULL,(void*(*)(void*))TcpServerConRecvThread,this))
    {
        sprintf(ErrorMsg,"%s","启动连接线程失败！");
        return -8;
    }

    if(0 != pthread_create(&m_SendThreadId,NULL,(void*(*)(void*))TcpServerSendThread,this))
    {
        sprintf(ErrorMsg,"%s","启动发送线程失败！");
        return -9;
    }

    return 0;
}


/*======================================================================//
函数功能：获取连接状态
输入参数：
    无
输出参数：
    无
返回：
    bool                             0：断开  1:连接
/=======================================================================*/
bool CTcpServer::GetConnectState()
{
    return m_bConnectState;
}

/*======================================================================//
函数功能：获取接收消息队列中的消息数量
输入参数：
    无
输出参数：
    无
返回：
    int                             消息数量 <0:错误
/=======================================================================*/
int CTcpServer::GetRecvMsgCount()
{
    int nRet = 0;
    nRet = msgctl(m_nRecvMsgid,IPC_STAT,&RecvMsgInfo);
    if(-1 == nRet)
    {
        sprintf(ErrorMsg,"%s","获取接收的消息数量错误");
        return -1;
    }

    return RecvMsgInfo.msg_qnum;
}

/*======================================================================//
函数功能：从消息队列获取一条数据
输入参数：
    unsigned char *ucDataBuffer     存放数据的缓存指针
    int nBufferSize                 缓存大小
输出参数：
    无
返回：
    int                             数据长度
                                    <0:错误
/=======================================================================*/
int CTcpServer::Recv(unsigned char *ucDataBuffer,int nBufferSize)
{
    if(0 >= GetRecvMsgCount())
    {
        sprintf(ErrorMsg,"%s","接收消息队列中消息数量<=0 ！");
        return -1;
    }
    struct msgbuf recv_temp;
    memset(&recv_temp,0,sizeof(struct msgbuf));
    int nRet = msgrcv(m_nRecvMsgid,&recv_temp, sizeof(recv_temp.data), 0,0);
    if(nRet > nBufferSize)
    {
        sprintf(ErrorMsg,"%s","消息大小大于给定的缓存！");
        return -2;
    }
    memcpy(ucDataBuffer,recv_temp.data,nRet);

    return nRet;
}

/*======================================================================//
函数功能：将需要发送的数据放入发送队列
输入参数：
    unsigned char *ucDataBuffer     需要发送的数据
    int nBufferSize                 需要发送的数据大小
输出参数：
    无
返回：
    int                             0：成功  <0:错误
/=======================================================================*/
int CTcpServer::Send(unsigned char *ucSendBuffer,int nBufferSize)
{
    struct msgbuf send_temp;
    memset(&send_temp,0,sizeof(struct msgbuf));
    send_temp.msg_type = 1;
    memcpy(send_temp.data,ucSendBuffer,nBufferSize);
    if(0 != msgsnd(m_nSendMsgid,&send_temp, nBufferSize, IPC_NOWAIT))
    {
        sprintf(ErrorMsg,"%s","向发送队列添加数据错误！");
        return -1;
    }

    return 0;
}

/*======================================================================//
函数功能:连接和接收线程。超时时间通过TcpServerInit()函数进行设置。接收到数据后会
         将数据放到接收消息队列。
输入参数：
    void *pParam
输出参数：
    无
返回：
    无
/=======================================================================*/
void CTcpServer::TcpServerConRecvThread(void *pParam)
{
    CTcpServer *pThis = (CTcpServer *)pParam;

    int nRet;
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);
    struct msgbuf st_RecvMsgBuf;
    while(1)
    {
        if(0 == pThis->m_bConnectState)
        {
            pThis->m_nConnnectId = accept(pThis->m_nTcpServerId, (struct sockaddr*)&client_addr, &length);
            if(pThis->m_nConnnectId < 0)
            {
                close(pThis->m_nConnnectId);
            }else
            {
                pThis->m_bConnectState = 1;
            }
        }else
        {
            memset(&st_RecvMsgBuf,0,sizeof (struct msgbuf));
            nRet = recv(pThis->m_nConnnectId,st_RecvMsgBuf.data,1024,0);
            if(0 < nRet)
            {
                st_RecvMsgBuf.msg_type = 1;
                if(0 != msgsnd(pThis->m_nRecvMsgid,&st_RecvMsgBuf, nRet, IPC_NOWAIT))
                {
                    //错误处理
                }
            }else
            {
                pThis->m_bConnectState = 0;
                close(pThis->m_nConnnectId);
            }
        }
        usleep(1000 *10);
    }
}

/*======================================================================//
函数功能:发送线程。从发送队列中取数据，通过tcp发送
输入参数：
    void *pParam
输出参数：
    无
返回：
    无
/=======================================================================*/
void CTcpServer::TcpServerSendThread(void *pParam)
{
    CTcpServer *pThis = (CTcpServer *)pParam;

    int nRet;
    struct msgbuf st_SendMsg;
    while(1)
    {
        while(1 == pThis->m_bConnectState)
        {
            memset(&st_SendMsg,0,sizeof(struct msgbuf));
            nRet = msgrcv(pThis->m_nSendMsgid,&st_SendMsg, sizeof(st_SendMsg.data), 0,0);
            if(0 < nRet)
            {
                send(pThis->m_nConnnectId,st_SendMsg.data,nRet,0);
            }
        }
        usleep(1000*100);
    }
}

