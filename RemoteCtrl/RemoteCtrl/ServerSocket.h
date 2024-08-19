#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
//���ݰ�
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
		//Ѱ�Ұ�ͷ
		size_t i = 0;
		for (;i < nSize;i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 8 > nSize) {	//������������
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i);
		i += 4;

		//�����󣬻��������ղ�ȫ
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
		//У��
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
	WORD sHead;	//��ͷ0xFEFF
	DWORD nLength;		//������(���������У��)
	WORD sCmd;	//��������
	std::string strData;	//����
	WORD sSum;	//��У��
	std::string strOut; //������
};
#pragma pack(pop)

//�������ṹ��
typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	//������ƶ���˫��
	WORD nButton;	//������Ҽ����м�
	POINT ptXY;	//����
}MOUSEEV, *PMOUSEEV;

//���쵥��ģʽ
class CServerSocket
{
public:
	//���þ�̬����,��̬����û�г�Աָ�룬�����޷����ʳ�Ա�������ֻ�ܷ��ʾ�̬����
	static CServerSocket* getInstance() {
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}
	//��ʼ��socket
	BOOL initSocket() {
		if (m_sock == -1) {
			return FALSE;
		}
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_port = htons(9527);
		//��
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
		//���û�����
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
			//TODO����������
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
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("�׽��ֳ�ʼ��ʧ��"), MB_OK | MB_ICONERROR);
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
