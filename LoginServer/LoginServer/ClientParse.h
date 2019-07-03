#pragma once
#include <string>
#include <map>

#include "Include/mysql/include/mysql.h"

#include "Include/rapidjson/rapidjson.h"
#include "Include/rapidjson/document.h"
#include "Include/rapidjson/prettywriter.h"  
#include "Include/rapidjson/stringbuffer.h" 
using namespace rapidjson;

#include <windows.h>

//�ͻ���ҵ���࣬һ�����Ӷ�Ӧһ���࣬����ֻ����ҵ�񣬲���������ͨ��
class CClientParse
{
public:
	CClientParse();
	~CClientParse();

public:
	//�����ͻ��˴������ַ�����Ϣ
	void Run(const std::string& strMsg);

	//��ȡҵ����Ĵ�����
	void GetReturnValue(char *pBuf, int nSize, unsigned long &nLength);

private:
	//ƴ�Ӵ�����Ϣ���ڷ���
	void ErrorMsg(const int nErrno = -1);

	//��ȡĬ�ϵ�ʹ������
	std::string GetDefaultExp();

	//У���Ƿ��ظ���¼
	void UpdateLoginState(const std::string& strUser);

private:
	//��¼
	void Login(Document& doc);

	//ע��
	void Register(Document& doc);

	//��������
	void ResetPassWord(Document& doc);

	//��ȡ�û�����
	void GetUserLevel(Document& doc);	

	//��ȡʹ��ʱ��
	void BeyondDeadLine(Document& doc);

	//�ظ���¼��У��
	void CheckRelogin(Document& doc);

private:
	std::string m_strResult;

	int m_nType;

	MYSQL *m_pSqlCon = nullptr;	//���ݿ�����

	std::string m_userName;		//�û���

	static CRITICAL_SECTION m_section;	//��

public:
	bool m_bReLogin = false;	//�Ƿ����ظ���¼

	static std::map<std::string, CClientParse*> s_ClientMap;	//�����û�����client��Ϣ�����ڴ�����

	static void DeleteSelfLoginState(CClientParse* pClient);	//�����ǰ�ĵ�¼״̬
};

