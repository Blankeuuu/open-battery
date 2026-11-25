#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <string>
// Development helper for graceful upgrades of older Open Battery versions.
// It refuses to send a close request to a different executable.
int wmain(int argc,wchar_t** argv){
    if(argc!=2)return 2;
    HWND window=FindWindowW(L"OpenBattery.Tray",L"Open Battery");if(!window){puts("No existing Open Battery window");return 0;}
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,pid);if(!process)return 3;
    wchar_t path[32768];DWORD size=32768;bool match=QueryFullProcessImageNameW(process,0,path,&size) && _wcsicmp(path,argv[1])==0;
    if(!match){CloseHandle(process);puts("Refused: executable path differs");return 4;}
    DWORD_PTR value=0;bool sent=SendMessageTimeoutW(window,WM_CLOSE,0,0,SMTO_ABORTIFHUNG,5000,&value)!=0;
    DWORD wait=sent?WaitForSingleObject(process,5000):WAIT_FAILED;CloseHandle(process);
    printf("Graceful close=%s\n",wait==WAIT_OBJECT_0?"ok":"failed");return wait==WAIT_OBJECT_0?0:5;
}
