// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <list>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

//调试输出包
void Dump(BYTE* pData, size_t nSize) {
    std::string strOut;
    for (size_t i = 0; i < nSize; i++)
    {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}

int MakeDriverInfo() { //1=>A 2=>B 3=>C...
    std::string result;
    for (size_t i = 1; i <= 26; i++)
    {
		//改变驱动
        if (_chdrive(i) == 0) {
            if (result.size() > 0) {
                result += ',';
            }
            result += 'A' + i - 1; //加上存在的驱动
        }
    }

    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    Dump((BYTE*)pack.Data() ,pack.Size());
//     CServerSocket::getInstance()->Send(pack);
    return 0;
}

//文件结构体
typedef struct file_info{
    file_info() {
        isInvalid = 0;
        isDirectory = -1;
        hasNext = TRUE;
        memset(szFileName, 0, sizeof(szFileName));
    }
    char szFileName[256];
    BOOL isDirectory;   //是否为目录 1是0否
    BOOL isInvalid; //是否有效
    BOOL hasNext;   //是否含有后续 0没有1有
}FILEINFO, *PFILEINFO;

//切片式发送
int MakeDirectoryInfo() {
    std::string strPath;
//     std::list<FILEINFO> listFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == FALSE) {
        OutputDebugString(_T("当前命令不是获取文件列表，命令解析错误!"));
        return -1;
    }
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.isInvalid = TRUE;
		finfo.isDirectory = TRUE;
        finfo.hasNext = FALSE;
        memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
//         listFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限访问目录或目录错误!"));
        return -2;
    }
    //查找文件
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到文件!"));
        return -3;
    }
    do 
    {
        FILEINFO finfo;
        finfo.isDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
// 		listFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));

    //最后发送一个NULL， FALSE, FALSE的fileinfo来表示结束
    FILEINFO finfo;
    finfo.hasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->Send(pack);

    return 0;
}

int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
	CPacket pack(3, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DownloadFile() {
	std::string strPath;
	CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pfile = NULL;
    errno_t err = fopen_s(&pfile, strPath.c_str(), "rb");
    if (err != 0) {
		CPacket pack(4, (BYTE*)&data, 8);
		CServerSocket::getInstance()->Send(pack);
		return -1;

    }
	if (pfile == NULL) {
        return -1;
	}
    //提前发送文件长度
    fseek(pfile, 0, SEEK_END);
    data = _ftelli64(pfile);
    CPacket head(4, (BYTE*)&data, 8);
    fseek(pfile, 0, SEEK_SET);

    char buffer[1024] = "";
    size_t rlen = 0;
    do 
    {
        rlen = fread(buffer, 1, 1024, pfile);
		CPacket pack(4, (BYTE*)buffer, rlen);
		CServerSocket::getInstance()->Send(pack);
    } while (rlen >= 1024);
    fclose(pfile);
	//发送结束标志
	CPacket pack(4, NULL, 0);
	CServerSocket::getInstance()->Send(pack);

}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            
//             CServerSocket* pserver =  CServerSocket::getInstance();
//             int count = 0;
// 			if (pserver->initSocket() == FALSE) {
// 				MessageBox(NULL, _T("initSocket失败"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
// 				exit(0);
// 			}
//             while (CServerSocket::getInstance() != NULL)
//             {
//                 if (pserver->acceptClient() == FALSE)
//                 {
//                     if (count > 4)
//                     {
// 						MessageBox(NULL, _T("acceptClient失败"), _T("连接客户端失败"), MB_OK | MB_ICONERROR);
// 						exit(0);
//                     }
// 					MessageBox(NULL, _T("acceptClient超时"), _T("重新连接客户端"), MB_OK | MB_ICONERROR);
//                     count++;
//                 }
//                 int ret = pserver->dealCommand();
//             }

            //实现功能
            int nCmd = 2;
            switch (nCmd)
            {
            case 1: //查看磁盘分区
				MakeDriverInfo();
                break;
            case 2: //查看指定目录下文件
                MakeDirectoryInfo();
                break;
            case 3: //打开文件
                RunFile();
            case 4: //下载文件
                DownloadFile();
            default:
                break;
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
