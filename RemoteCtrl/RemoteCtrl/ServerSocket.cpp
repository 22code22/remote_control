#include "pch.h"
#include "ServerSocket.h"

// CServerSocket server;

//显式方法初始化
CServerSocket*  CServerSocket::m_instance = NULL;

CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();