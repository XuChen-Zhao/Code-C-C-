/*˵����
 * 1�����ĵ�����TCPЭ��������˻��ࡣ
 * 2��ִ�г�ʼ���󣬻��Զ��������ӡ����ա����͡�
 * 3�������������յ����ݺ󣬽����ݷŵ�������Ϣ���С�����Ҫ��ȡ��������ʱ������Recvֱ�ӻ�ȡ��
 * 4������Ҫ��������ʱ�����Ƚ����ݷ��뷢����Ϣ���С������̴߳ӷ�����Ϣ���л�ȡ���ݣ�Ȼ���͡�
 * 5�����б���ErrorMsg���ڱ������һ�δ�����Ϣ�����������󣬿����̵��ý��в鿴��
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
    int m_nOutTime;         //��ʱʱ�䡣����ʱ����û�н��յ����ݣ���Ͽ���
    bool m_bConnectState;   //����״̬
    int m_nTcpServerId;     //socket���
    int m_nConnnectId;      //���Ӿ��
    int m_nSendMsgid;       //������Ϣ����
    int m_nRecvMsgid;       //������Ϣ����
    struct msqid_ds RecvMsgInfo;    //���ڻ�ȡ������Ϣ������Ϣ

    pthread_t m_ConRecvThreadId;    //���Ӻͽ����߳�
    pthread_t m_SendThreadId;       //�����߳�

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
