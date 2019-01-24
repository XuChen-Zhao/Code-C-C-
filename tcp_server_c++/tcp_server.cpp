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
�������ܣ��ú����ж�tcp����������г�ʼ����������ʼ�����Ͷ��кͽ��ն��У�socket��ʼ����
            ����̳߳�ʼ����
���������
    int nPort                       �˿ں�
    int nOutTime                    ��ʱʱ��
���������
    ��
���أ�
    int                             0����ȷ  <0:����
/=======================================================================*/
int CTcpServer::TcpServerInit(int nPort,int nOutTime)
{
    if((0 >= nPort) || (0 >= nOutTime))
    {
        sprintf(ErrorMsg,"%s","�˿ںŻ�ʱʱ�����ô���");
        return -1;
    }
    //������Ϣ���г�ʼ��
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
        sprintf(ErrorMsg,"%s","�������Ͷ���ʧ��");
        return -2;
    }
    //������Ϣ���г�ʼ��
    key = ftok(".",'a');
    m_nRecvMsgid = msgget(key,IPC_CREAT|IPC_EXCL|0666);
    if((-1 == m_nRecvMsgid) && (errno == EEXIST))
    {
        key=ftok(".",'a');
        m_nRecvMsgid = msgget(key, IPC_CREAT|0666);
    }
    if(0 > m_nRecvMsgid)
    {
        sprintf(ErrorMsg,"%s","�������ն���ʧ��");
        return -3;
    }

    //��������ʼ��
    m_nTcpServerId = socket(AF_INET,SOCK_STREAM, 0);
    if(0 >= m_nTcpServerId)
    {
        sprintf(ErrorMsg,"%s","����socketʧ��");
        return -4;
    }

    struct timeval timeout={nOutTime,0};
    if(0 != setsockopt(m_nTcpServerId,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout)))
    {
        sprintf(ErrorMsg,"%s","���ó�ʱʱ��ʧ��");
        return -5;
    }

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(nPort);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(0 != bind(m_nTcpServerId,(struct sockaddr *)&server_sockaddr,sizeof(server_sockaddr)))
    {
        sprintf(ErrorMsg,"%s","bindʧ��");
        return -6;
    }

    if(0 != listen(m_nTcpServerId,20))
    {
        sprintf(ErrorMsg,"%s","listenʧ��");
        return -7;
    }

    //��ͨ���Ӻͽ����߳�
    if(0 != pthread_create(&m_ConRecvThreadId,NULL,(void*(*)(void*))TcpServerConRecvThread,this))
    {
        sprintf(ErrorMsg,"%s","���������߳�ʧ�ܣ�");
        return -8;
    }

    if(0 != pthread_create(&m_SendThreadId,NULL,(void*(*)(void*))TcpServerSendThread,this))
    {
        sprintf(ErrorMsg,"%s","���������߳�ʧ�ܣ�");
        return -9;
    }

    return 0;
}


/*======================================================================//
�������ܣ���ȡ����״̬
���������
    ��
���������
    ��
���أ�
    bool                             0���Ͽ�  1:����
/=======================================================================*/
bool CTcpServer::GetConnectState()
{
    return m_bConnectState;
}

/*======================================================================//
�������ܣ���ȡ������Ϣ�����е���Ϣ����
���������
    ��
���������
    ��
���أ�
    int                             ��Ϣ���� <0:����
/=======================================================================*/
int CTcpServer::GetRecvMsgCount()
{
    int nRet = 0;
    nRet = msgctl(m_nRecvMsgid,IPC_STAT,&RecvMsgInfo);
    if(-1 == nRet)
    {
        sprintf(ErrorMsg,"%s","��ȡ���յ���Ϣ��������");
        return -1;
    }

    return RecvMsgInfo.msg_qnum;
}

/*======================================================================//
�������ܣ�����Ϣ���л�ȡһ������
���������
    unsigned char *ucDataBuffer     ������ݵĻ���ָ��
    int nBufferSize                 �����С
���������
    ��
���أ�
    int                             ���ݳ���
                                    <0:����
/=======================================================================*/
int CTcpServer::Recv(unsigned char *ucDataBuffer,int nBufferSize)
{
    if(0 >= GetRecvMsgCount())
    {
        sprintf(ErrorMsg,"%s","������Ϣ��������Ϣ����<=0 ��");
        return -1;
    }
    struct msgbuf recv_temp;
    memset(&recv_temp,0,sizeof(struct msgbuf));
    int nRet = msgrcv(m_nRecvMsgid,&recv_temp, sizeof(recv_temp.data), 0,0);
    if(nRet > nBufferSize)
    {
        sprintf(ErrorMsg,"%s","��Ϣ��С���ڸ����Ļ��棡");
        return -2;
    }
    memcpy(ucDataBuffer,recv_temp.data,nRet);

    return nRet;
}

/*======================================================================//
�������ܣ�����Ҫ���͵����ݷ��뷢�Ͷ���
���������
    unsigned char *ucDataBuffer     ��Ҫ���͵�����
    int nBufferSize                 ��Ҫ���͵����ݴ�С
���������
    ��
���أ�
    int                             0���ɹ�  <0:����
/=======================================================================*/
int CTcpServer::Send(unsigned char *ucSendBuffer,int nBufferSize)
{
    struct msgbuf send_temp;
    memset(&send_temp,0,sizeof(struct msgbuf));
    send_temp.msg_type = 1;
    memcpy(send_temp.data,ucSendBuffer,nBufferSize);
    if(0 != msgsnd(m_nSendMsgid,&send_temp, nBufferSize, IPC_NOWAIT))
    {
        sprintf(ErrorMsg,"%s","���Ͷ���������ݴ���");
        return -1;
    }

    return 0;
}

/*======================================================================//
��������:���Ӻͽ����̡߳���ʱʱ��ͨ��TcpServerInit()�����������á����յ����ݺ��
         �����ݷŵ�������Ϣ���С�
���������
    void *pParam
���������
    ��
���أ�
    ��
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
                    //������
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
��������:�����̡߳��ӷ��Ͷ�����ȡ���ݣ�ͨ��tcp����
���������
    void *pParam
���������
    ��
���أ�
    ��
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

