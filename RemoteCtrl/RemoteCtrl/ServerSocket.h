#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
//数据包
class CPacket
{
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pdata, size_t nsize) {
		sHead = 0xFEFF;
		nLength = nsize + 4;
		sCmd = nCmd;
		if (nsize > 0) {
			strData.resize(nsize);
			memcpy((BYTE*)strData.c_str(), pdata, nsize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0XFF;
		}

	}
	CPacket(const CPacket& packet){
		sHead = packet.sHead;
		nLength = packet.nLength;
		strData = packet.strData;
		sCmd = packet.sCmd;
		sSum = packet.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) {
		//寻找包头
		size_t i = 0;
		for (;i < nSize;i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 8 > nSize) {	//加上其他部分
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i);
		i += 4;

		//包过大，缓存区接收不全
		if (nLength + i > nSize) {
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength < 4) {
			nSize = 0;
			return;
		}
		strData.resize(nLength - 2 - 2);
		memcpy((void*)strData.c_str(), pData + i, nLength - 4);
		i += nLength - 4;

		sSum = *(WORD*)(pData + i);
		i += 2;
		//校验
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0XFF;
		}
		if (sum == sSum) {
			nSize = i;
			return;
		}
		nSize = 0;
	}

	CPacket& operator=(const CPacket& packet){
		if (this != &packet)
		{
			sHead = packet.sHead;
			nLength = packet.nLength;
			strData = packet.strData;
			sCmd = packet.sCmd;
			sSum = packet.sSum;
		}
		return *this;
	}

	int Size() {
		return nLength + 6;
	}

	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)pData = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}

	~CPacket() {}

public:
	WORD sHead;	//包头0xFEFF
	DWORD nLength;		//包长度(控制命令到和校验)
	WORD sCmd;	//命令类型
	std::string strData;	//内容
	WORD sSum;	//和校验
	std::string strOut; //整个包
};
#pragma pack(pop)

//鼠标操作结构体
typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	//点击，移动，双击
	WORD nButton;	//左键，右键，中键
	POINT ptXY;	//坐标
}MOUSEEV, *PMOUSEEV;

//构造单例模式
class CServerSocket
{
public:
	//设置静态函数,静态函数没有成员指针，所以无法访问成员遍变量，只能访问静态变量
	static CServerSocket* getInstance() {
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	//初始化socket
	BOOL initSocket() {
		if (m_sock == -1) {
			return FALSE;
		}
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_port = htons(9527);
		//绑定
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			return FALSE;
		}
		if (listen(m_sock, 1) == -1) {
			return FALSE;
		}
		return TRUE;
	}

	BOOL acceptClient() {
		sockaddr_in client_adr;
		int cli_size = sizeof(client_adr);

		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_size);
		if (m_client == -1) {
			return FALSE;
		}
		return TRUE;
	}

#define BUFFER_SIZE 4096
	int dealCommand() {
		if (m_client == -1) {
			return -1;
		}
		//设置缓冲区
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer, BUFFER_SIZE - index, 0);
			if (len <= 0)
			{
				return -1;
			}
			index += len;
			len = index;
			//TODO：处理命令
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	BOOL Send(const char* pData, size_t nSize) {
		if (m_client == -1) {
			return FALSE;
		}

		return send(m_client, pData, nSize, 0);
	}
	BOOL Send(CPacket& pack) {
		if (m_client == -1) {
			return FALSE;
		}
		return send(m_client, pack.Data(), pack.Size(), 0);

	}
	BOOL GetFilePath(std::string& strPath) {
		if (m_packet.sCmd == 2 || m_packet.sCmd == 3 || m_packet.sCmd == 4) {
			strPath = m_packet.strData;
			return TRUE;
		}
		return FALSE;
	}
	BOOL GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return TRUE;
		}
	}
private:
	SOCKET m_sock, m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {

	}

	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {
		m_sock = INVALID_SOCKET;
		m_client = INVALID_SOCKET;
		if (init_socket_env() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("套接字初始化失败"), MB_OK | MB_ICONERROR);
			exit(0);
		}	
		m_sock = socket(PF_INET, SOCK_STREAM, 0);

	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}

	BOOL init_socket_env() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}

		return TRUE;
	}
	static void releaseInstance() {
		if (m_instance != NULL)
		{
			CServerSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;
		}
	}
	static CServerSocket* m_instance;
	class CHelper
	{
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};

extern CServerSocket server;
