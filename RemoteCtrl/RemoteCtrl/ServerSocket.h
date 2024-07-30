#pragma once
#include "pch.h"
#include "framework.h"

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

	int dealCommand() {
		if (m_client == -1) {
			return -1;
		}
		char buffer[1024] = "";
		while (true)
		{
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0)
			{
				return -1;
			}
			//TODO����������
		}
	}

	BOOL Send(const char* pData, size_t nSize) {
		if (m_client == -1) {
			return FALSE;
		}

		return send(m_client, pData, nSize, 0);
	}
private:
	SOCKET m_sock, m_client;
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
