#include "pch.h"
#include "ServerSocket.h"

// CServerSocket server;

//��ʽ������ʼ��
CServerSocket*  CServerSocket::m_instance = NULL;

CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();