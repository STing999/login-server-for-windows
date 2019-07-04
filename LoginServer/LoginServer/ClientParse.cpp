#include "pch.h"
#include "ClientParse.h"
#include "logger.h"

#pragma comment(lib, "../Include/mysql/lib/libmysql.lib")

#define DB_IP		"localhost"
#define DB_USER		"root"
#define DB_PWD		"575619"
#define DB_NAME		"login"
#define TABLE_NAME	"USER"

CClientParse::CClientParse()
{
	InitializeCriticalSection(&m_section);

	m_nType = CS_UNDEFINED;

	m_pSqlCon = mysql_init((MYSQL*)0);
	if (m_pSqlCon != NULL && mysql_real_connect(m_pSqlCon, DB_IP, DB_USER, DB_PWD, DB_NAME, 3306, NULL, 0))
	{
		if (!mysql_select_db(m_pSqlCon, DB_NAME)) 
		{
			INFO_LOGGER << "Select successfully the database!" << END_LOGGER;
			m_pSqlCon->reconnect = 1;
		}
	}
	else 
	{
		std::string strErr = mysql_error(m_pSqlCon);
		WARNNING_LOGGER << "Connect Database Error " << strErr << END_LOGGER;
		m_pSqlCon = NULL;
	}
}


CClientParse::~CClientParse()
{
	DeleteCriticalSection(&m_section);

	if (m_pSqlCon)
	{
		mysql_close(m_pSqlCon);
	}
}

void CClientParse::Run(const std::string& strMsg)
{
	m_nType = CS_UNDEFINED;

	if (strMsg.empty())
		return;

	//json����
	Document document;
	document.Parse((char*)strMsg.c_str(), strMsg.length());
	if (document.HasParseError())
	{
		ErrorMsg(ERROR_PARSE_JSON);
		//WARNNING_LOGGER << "����json����json��Ϊ;" << strMsg << END_LOGGER;
		return;
	}

	if (!document.IsObject())
	{
		ErrorMsg(ERROR_PARSE_JSON);
		//WARNNING_LOGGER << "�����Ǳ�׼��json��" << END_LOGGER;
		return;
	}

	//��ȡҵ������
	if (document.HasMember("type") && document["type"].IsInt())
	{
		m_nType = document["type"].GetInt();
	}
	if (m_nType == CS_UNDEFINED)
	{
		//WARNNING_LOGGER << "ҵ������Ϊ: " << m_nType << "����֧��" << END_LOGGER;
		return;
	}

	//std::cout << m_nType << " begin  :" << this  << " " << m_strResult << std::endl;

	//std::cout << m_nType << std::endl;

	//����ͬ��ҵ��
	switch (m_nType)
	{
	case CS_LOGIN:
		Login(document);
		break;
	case CS_REGISTER:
		Register(document);
		break;
	case CS_RESETPASSWORD:
		ResetPassWord(document);
		break;
	case CS_BEYONDDEADLINE:
		BeyondDeadLine(document);
		break;
	case CS_GETUSERLEVEL:
		GetUserLevel(document);
		break;
	case CS_RELOGINTHREAD:
		CheckRelogin(document);
		break;
	case CS_HEARTTHREAD:
		m_strResult = "10000";
		break;
	default:
		break;
	}

	//std::cout << m_nType << " end  :" << this << " " << m_strResult << std::endl;
}

void CClientParse::GetReturnValue(char *pBuf, int nSize, unsigned long &nLength)
{
	if (pBuf == NULL)
		return;

	memset(pBuf, 0, nSize);

	if (m_strResult.length() <= 0)
		return;

	//����������
	if (m_strResult.length() > nSize - 1)
	{
		strcpy_s(pBuf, nSize -1, m_strResult.c_str());
		nLength = nSize - 1;
	}
	else
	{
		strcpy_s(pBuf, m_strResult.length() + 1, m_strResult.c_str());
		nLength = m_strResult.length() + 1;
	}
	 
}

void CClientParse::ErrorMsg(const int nErrno /* = ERROR_UNKNOWN*/)
{
	GenericStringBuffer<ASCII<> > JsonStringBuff;
	Writer<GenericStringBuffer<ASCII<> >  > JsonWriter(JsonStringBuff);

	JsonWriter.StartObject();
	JsonWriter.Key("return_code");
	JsonWriter.Int(nErrno);
	JsonWriter.EndObject();

	m_strResult = JsonStringBuff.GetString();
}

std::string CClientParse::GetDefaultExp()
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	int y = st.wYear + 1;	 //��ȡ�����+1
	int m = st.wMonth;		 //��ȡ��ǰ�·�
	int d = st.wDay;		 //��ü���
	char buf[64] = { 0 };
	sprintf_s(buf, "%d%02d%02d", y, m, d);
	return buf;
}

void CClientParse::UpdateLoginState(const std::string& strUser)
{
	EnterCriticalSection(&m_section);

	auto it = s_ClientMap.find(strUser);
	if (it != s_ClientMap.end())
	{
		//�Ѿ���¼��,����value
		it->second->m_bReLogin = true;
		s_ClientMap[strUser] = this;
	}
	else
	{
		//û�е�¼������
		s_ClientMap[strUser] = this;
	}

	LeaveCriticalSection(&m_section);
}

void CClientParse::Login(Document& doc)
{
	if (!doc.HasMember("userName") || !doc["userName"].IsString())
	{
		WARNNING_LOGGER << "�û�����Ϣ����ȷ" << END_LOGGER;
		return;
	}

	if (!doc.HasMember("passWord") || !doc["passWord"].IsString())
	{
		WARNNING_LOGGER << "������Ϣ����ȷ" << END_LOGGER;
		return;
	}

	std::string userName = doc["userName"].GetString();
	std::string sql = "SELECT NAME from USER WHERE NAME=\'" + userName + "\'";
	bool existUserName = false;
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "2003";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//�����������res�ṹ����
	if (!mysql_fetch_row(res))
	{
		m_strResult = "2001";
		INFO_LOGGER << "�û�: " << userName << " ������" << END_LOGGER;
		return;
	}

	std::string passWord = doc["passWord"].GetString();
	sql = "SELECT * from USER WHERE NAME=\'" + userName + "\' and PASSWORD=\'" + passWord + "\'";
	bool exist = false;
	rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT * from USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "2003";
		return;
	}
	else
	{
		MYSQL_RES *res = mysql_store_result(m_pSqlCon);//�����������res�ṹ����
		if (mysql_fetch_row(res))
		{
			m_userName = userName;
			m_strResult = "2000";

			UpdateLoginState(m_userName);

			INFO_LOGGER << userName << ":��½�ɹ�: " << END_LOGGER;
		}
		else
		{
			m_strResult = "2002";
			WARNNING_LOGGER << "�˺�:" << userName << " ����:" << passWord << " ����ȷ" << END_LOGGER;
		}
	}
}

void CClientParse::Register(Document& doc)
{
	std::string userName, passWord, qqCount, expDate, level, userType;

	//�û����������
	if (doc.HasMember("userName") && doc["userName"].IsString())
	{
		userName = doc["userName"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "�û���������" << END_LOGGER;
		return;
	}

	//����������
	if (doc.HasMember("passWord") && doc["passWord"].IsString())
	{
		passWord = doc["passWord"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "�û���������" << END_LOGGER;
		return;
	}

	if (doc.HasMember("qqCount") && doc["qqCount"].IsString())
	{
		qqCount = doc["qqCount"].GetString();
	}

	if (doc.HasMember("exp") && doc["exp"].IsString())
	{
		expDate = doc["exp"].GetString();
	}
	
	if (doc.HasMember("level") && doc["level"].IsString())
	{
		level = doc["level"].GetString();
	}

	if (doc.HasMember("userType") && doc["userType"].IsString())
	{
		userType = doc["userType"].GetString();
	}

	//�ȿ��Ƿ��˺��Ѵ���
	std::string sql = "SELECT NAME from USER WHERE NAME=\'" + userName + "\'";
	bool existUserName = false;
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "1001";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//�����������res�ṹ����
	if (mysql_fetch_row(res))
	{
		m_strResult = "1002";
		INFO_LOGGER << "�˺��Ѵ��ڣ��볢��ѡ�������˺�ע�ᣡ: " << userName << END_LOGGER;
		return;
	}

	//��������ע��
	std::string expTime = GetDefaultExp();
	//sql = "INSERT INTO USER (NAME, PASSWORD, QQCOUNT, EXP) VALUES (\'" + userName + "', '" + passWord + +"', '" + qqCount + "', '" + expTime + "')";
	sql = "INSERT INTO USER (NAME, PASSWORD, QQCOUNT, EXP, LEVEL, USERTYPE) VALUES (\'" + \
		userName + "', '" + passWord + "', '" + qqCount + "', '" + expTime + "', '" + level + "', '" + userType + "')";
	rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "1003";
		return;
	}
	m_strResult = "1000";
	INFO_LOGGER << "�˺�ע��ɹ�: " << userName << END_LOGGER;
}

void CClientParse::ResetPassWord(Document& doc)
{
	std::string userName, oldPassWord, newPassWord;

	//�û����������
	if (doc.HasMember("userName") && doc["userName"].IsString())
	{
		userName = doc["userName"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "�û���������" << END_LOGGER;
		return;
	}

	//����������
	if (doc.HasMember("oldPassWord") && doc["oldPassWord"].IsString())
	{
		oldPassWord = doc["oldPassWord"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "�����벻����" << END_LOGGER;
		return;
	}

	//������������
	if (doc.HasMember("newPassWord") && doc["newPassWord"].IsString())
	{
		newPassWord = doc["newPassWord"].GetString();
	}
	else
	{
		WARNNING_LOGGER << "�����벻����" << END_LOGGER;
		return;
	}

	std::string sql = "SELECT PASSWORD from USER WHERE NAME=\'" + userName + "\'";
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		WARNNING_LOGGER << "SELECT NAME USER WHERE NAME= " << userName << " Failed:" << END_LOGGER;
		m_strResult = "5001";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//�����������res�ṹ����
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "5004";
		INFO_LOGGER << "�û�: " << userName << " ������" << END_LOGGER;
		return;
	}

	//�ȽϿ��е�����ʹ�����������
	std::string strData = row[0];
	if (strData.compare(oldPassWord) != 0)
	{
		INFO_LOGGER << "����ľ����벻��ȷ:" << userName << END_LOGGER;
		m_strResult = "5002";
		return;
	}

	//��������
	sql = "UPDATE USER SET PASSWORD=\'" + newPassWord + "\'" + \
		"WHERE NAME=\'" + userName + "\'";;
	rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		ERROR_LOGGER << "UPDATE USER SET PASSWORD: " << userName << " Failed" << END_LOGGER;
		m_strResult = "5003";
	}
	m_strResult = "0";
	INFO_LOGGER << "RESET PASSWORD SUCCEED: " << userName << END_LOGGER;
}

void CClientParse::GetUserLevel(Document& doc)
{
	if (m_userName.empty())
	{
		m_strResult = "-1";
		return;
	}

	std::string sql = "SELECT LEVEL from USER WHERE NAME=\'" + m_userName + "\'";
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		//WARNNING_LOGGER << "SELECT LEVEL USER WHERE NAME= " << m_userName << " Failed:" << END_LOGGER;
		m_strResult = "2003";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//�����������res�ṹ����
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "2001";
		//INFO_LOGGER << "�û�: " << m_userName << " ������" << END_LOGGER;
		return;
	}

	//�����ֵ
	m_strResult = row[0];
	INFO_LOGGER << "�û�:" << m_userName << " �ȼ�: " << m_strResult << END_LOGGER;
}

void CClientParse::BeyondDeadLine(Document& doc)
{
	if (m_userName.empty())
	{
		m_strResult = "-1";
		return;
	}

	std::string sql = "SELECT EXP from USER WHERE NAME=\'" + m_userName + "\'";
	int rc = mysql_real_query(m_pSqlCon, sql.c_str(), sql.length());
	if (rc != 0)
	{
		//WARNNING_LOGGER << "SELECT LEVEL USER WHERE NAME= " << m_userName << " Failed:" << END_LOGGER;
		m_strResult = "3";
		return;
	}
	MYSQL_RES *res = mysql_store_result(m_pSqlCon);//�����������res�ṹ����
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		m_strResult = "2001";
		//INFO_LOGGER << "�û�: " << m_userName << " ������" << END_LOGGER;
		return;
	}

	int expDateVal = atoi(row[0]);
	SYSTEMTIME st;
	GetSystemTime(&st);
	int y = st.wYear;  //��ȡ�����
	int m = st.wMonth; //��ȡ��ǰ�·�
	int d = st.wDay;   //��ü���
	char buf[64] = { 0 };
	sprintf_s(buf, "%d%02d%02d", y, m, d);
	int curDate = atoi(buf);
	int subVal = expDateVal - curDate;
	if (subVal >= 0)
	{
		m_strResult = "0";
	}
	else
	{
		m_strResult = "1";
	}
}

void CClientParse::CheckRelogin(Document& doc)
{
	if (m_bReLogin)
	{
		m_strResult = "1";
		INFO_LOGGER << "��⵽�û��ظ���½��ǿ�ƹرգ�" << m_userName << END_LOGGER;
	}
	else
	{
		m_strResult = "0";
		//INFO_LOGGER << "�ظ���¼������" << END_LOGGER;
	}
}

CRITICAL_SECTION CClientParse::m_section;

std::map<std::string, CClientParse*> CClientParse::s_ClientMap;

void CClientParse::DeleteSelfLoginState(CClientParse* pClient)
{
	EnterCriticalSection(&m_section);

	auto it = s_ClientMap.begin();
	for (; it != s_ClientMap.end(); it++)
	{
		if (it->second == pClient)
		{
			s_ClientMap.erase(it);
			break;
		}
	}

	LeaveCriticalSection(&m_section);
}
