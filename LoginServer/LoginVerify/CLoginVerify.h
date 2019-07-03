#pragma once
#include <iostream>
#include "..\json\json.h"
#include <WinSock2.h>
enum USER_TYPE {
    NON,
    STOCK,
    FUTURES
};
enum USER_LEVEL{
    NONE,
    PRIMARY,
    INTERMEDIATE,
    SENIOR
};
class CLoginVerify
{
public:
    CLoginVerify();
    // ��ʼ��env
    int InitEnv(Json::Value& serverInfo);
    // �û���½
    virtual int Login(Json::Value& userInfo);
    // �û�ע��
    virtual int Register(Json::Value& userInfo);
    // ��ȡ���°汾�ͻ�����Ϣ
    virtual const std::string GetLatestVersion();
    // ��ȡϵͳʱ��
    virtual const std::string GetSystemTime();
    // ��ȡ�û���ֹ����
    virtual const std::string GetDeadLine();
    // �ж��û��Ƿ�ʧЧ
    virtual bool BeyondDeadLine();
    // ��ȡ�û�����
    virtual USER_TYPE GetUserType();
    // ��ȡ�û��ȼ�
    virtual USER_LEVEL GetUserLevel();
    // �����û���ֹ����
    virtual int SetDeadLine(const Json::Value& sendDeadLine);
    // ����env
    void cleanEnv();
    // ��������
    void KeepAlive();
    // Get socket
    SOCKET GetSocket();
    //Set Socket
    void SetSocket(SOCKET sock);
    //Reset password
    virtual int ResetPassWord(Json::Value& newPassWd);
public:
    std::string RecvMes();
    void SendJsonMes(const Json::Value& sendJsonMes);
    void SendStrMes(std::string sendStrMes);
    int _keepAlive;

private:
    void CreateLog();
    std::string _userName;
    SOCKET _clientSocket;
};