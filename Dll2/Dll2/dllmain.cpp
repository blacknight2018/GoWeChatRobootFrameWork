// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "server.h"

HMODULE WeChatWinModule;
DWORD dwRecvFunc;
typedef int(__stdcall* ptrSrc)(int,int,int);

void sendMsgToClient(string wxid, string wxmsg);
void startListenServer();

std::wstring String2WString(const std::string& s);
std::string WString2String(const std::wstring& ws);

/// <summary>
/// 发送文本消息
/// </summary>
void Call_SendTextMessage(wstring wxid,wstring msg)
{
    struct UnicodeStruct
    {
        wchar_t*  ptr;
        int len;
    };
    UnicodeStruct* id = new UnicodeStruct();
    id->ptr = (wchar_t*)wxid.data();
    id->len = wxid.size();
    UnicodeStruct* text = new UnicodeStruct();
    text->ptr = (wchar_t*)msg.data();
    text->len = msg.size();
    char buff[2000]{ 0 };
    int function = 0x2EB4E0 +(int) WeChatWinModule;
    __asm {
        pushad;
        pushfd;

        mov edx, id;
        push 1;
        push 0;
        push text;
        lea ecx, buff;
        call function;
        add esp, 0xc;

        popfd;
        popad;
    }

}

/// <summary>
/// 接收文本消息
/// </summary>
void Call_RecvTextMessage(const wstring wxid, const wstring msg)
{
    sendMsgToClient(WString2String(wxid), WString2String(msg));
}


int __stdcall BeHooked(int a,int b,int c)
{
    int *e;
    __asm {
        mov e,ebx
    }
    //(*e) + 0x40 -> wxid
    //(*e) + 0x68 -> wxmg
    if ( ((int)e )>= 100000){
       Call_RecvTextMessage((LPCWSTR)(*(int*)(*e + 0x40)), (LPCWSTR)(*(int*)(*e + 0x68)));
    }

   
    return ((ptrSrc)dwRecvFunc)(a,b,c);
}

void Enter()
{
    //WeChatWin.dll + 0x2C9650
    WeChatWinModule = LoadLibraryA("WeChatWin.dll");
    if (WeChatWinModule == 0) {
        //MessageBoxA(NULL, "WeChatWin.dll Not Loaded", "", MB_OK);
        return;
    }

    
    //MessageBoxA(NULL, "WeChatWin.dll has Loaded", "", MB_OK);
    //CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Test, NULL, NULL,  NULL);

    //不知道这两个什么鸟用
    dwRecvFunc = (int)WeChatWinModule + 0x2CAEE0;
    
    DetourRestoreAfterWith();
    DetourTransactionBegin();

    DetourUpdateThread(GetCurrentThread());

    DetourAttach(&(PVOID&)dwRecvFunc, BeHooked);
    DetourTransactionCommit();
    
    
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 0), &wsaData);
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)startListenServer, NULL, NULL, NULL);
        Enter();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


void startListenServer()
{
    SOCKET localSer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (localSer != 0)
    {
        struct sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_port = htons(27015); ///监听端口   
        local.sin_addr.s_addr = INADDR_ANY; ///本机
        bind(localSer, (struct sockaddr*)&local, sizeof(local));

        struct sockaddr_in from;//客户端地址相关结构体
        int fromlen = sizeof(from);
        int len;

        MessageBoxA(NULL, "启动服务成功", "", MB_OK);
        if (0 == listen(localSer, 0)) {

            while (true)
            {
                SOCKET sockConn = accept(localSer, (SOCKADDR*)&from, &fromlen);
                recv(sockConn, (char*)&len, sizeof(int), 0);
                len = ntohl(len);
                char* buff = new char[len + 1];

                recv(sockConn, buff, len, 0);
                buff[len] = 0;

                recv(sockConn, (char*)&len, sizeof(int), 0);
                len = ntohl(len);
                char* buff2 = new char[len + 1];
                recv(sockConn, buff2, len, 0);
                buff2[len] = 0;

                string wxid(buff);
                string wxmsg(buff2);

                //Need Send
                cout << wxid << wxmsg << endl;
                
                Call_SendTextMessage(String2WString(wxid), String2WString(wxmsg));

                delete[]buff;
                delete[]buff2;
                closesocket(sockConn);
            }
        }
    }
}

void sendMsgToClient(string wxid, string wxmsg)
{
    SOCKET localCli = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (localCli != 0)
    {
        sockaddr_in serAddr;
        serAddr.sin_family = AF_INET;
        serAddr.sin_port = htons(8777);
        serAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
        if (connect(localCli, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
            return;
        }
        int len = htonl(wxid.size());
        send(localCli, (char*)&len, sizeof(int), 0);
        send(localCli, wxid.data(), wxid.size(), 0);

        len = htonl(wxmsg.size());
        send(localCli, (char*)&len, sizeof(int), 0);
        send(localCli, wxmsg.data(), wxmsg.size(), 0);

        closesocket(localCli);


    }
}

//wstring=>string
std::string WString2String(const std::wstring& ws)
{
    std::string strLocale = setlocale(LC_ALL, "");
    const wchar_t* wchSrc = ws.c_str();
    size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
    char* chDest = new char[nDestSize];
    memset(chDest, 0, nDestSize);
    wcstombs(chDest, wchSrc, nDestSize);
    std::string strResult = chDest;
    delete[]chDest;
    setlocale(LC_ALL, strLocale.c_str());
    return strResult;
}
// string => wstring
std::wstring String2WString(const std::string& s)
{
    std::string strLocale = setlocale(LC_ALL, "");
    const char* chSrc = s.c_str();
    size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
    wchar_t* wchDest = new wchar_t[nDestSize];
    wmemset(wchDest, 0, nDestSize);
    mbstowcs(wchDest, chSrc, nDestSize);
    std::wstring wstrResult = wchDest;
    delete[]wchDest;
    setlocale(LC_ALL, strLocale.c_str());
    return wstrResult;
}