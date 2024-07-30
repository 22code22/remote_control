#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	CServerSocket() {
		if (init_socket_env() == FALSE)
		{
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ���"), _T("�׽��ֳ�ʼ��ʧ��"), MB_OK | MB_ICONERROR);
			exit(0);
		}

	}
	~CServerSocket() {
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
private:

};

extern CServerSocket server;
