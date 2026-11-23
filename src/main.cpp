#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <cstdio>
#include <string>
#include <vector>
#include "hid.hpp"
#include "icons.hpp"
#include "number-icon.hpp"
#include "../build/version.hpp"

namespace {
constexpr wchar_t product[]=L"Open Battery";
constexpr wchar_t settingsKey[]=L"Software\\Open Battery";
constexpr wchar_t runKey[]=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr UINT trayMessage=WM_APP+1, resultMessage=WM_APP+2;
constexpr UINT refreshId=100, autoId=101, copyId=102, debugId=103, logsId=104, exitId=105;
HWND window=nullptr;
HANDLE worker=nullptr;
HICON icon=nullptr;
UINT taskbarCreated=0;
bool debug=false, initialized=false;
ob::Alert alert;
ob::Reading current;
int lastPercent=-1;
SYSTEMTIME lastTime{};
std::wstring folder, logFolder;

DWORD readSetting(const wchar_t* name,DWORD fallback=0){
    DWORD value=fallback,size=sizeof(value);
    RegGetValueW(HKEY_CURRENT_USER,settingsKey,name,RRF_RT_REG_DWORD,nullptr,&value,&size);
    return value;
}
void writeSetting(const wchar_t* name,DWORD value){
    HKEY key=nullptr;
    if(RegCreateKeyExW(HKEY_CURRENT_USER,settingsKey,0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)==ERROR_SUCCESS){
        RegSetValueExW(key,name,0,REG_DWORD,reinterpret_cast<BYTE*>(&value),sizeof(value));RegCloseKey(key);
    }
}
bool autoStart(){
    DWORD size=0;
    return RegGetValueW(HKEY_CURRENT_USER,runKey,product,RRF_RT_REG_SZ,nullptr,nullptr,&size)==ERROR_SUCCESS;
}
bool setAutoStart(bool enabled){
    HKEY key=nullptr;
    if(RegCreateKeyExW(HKEY_CURRENT_USER,runKey,0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS) return false;
    LSTATUS status=ERROR_SUCCESS;
    if(enabled){
        wchar_t path[32768];GetModuleFileNameW(nullptr,path,32768);
        std::wstring command=L"\""+std::wstring(path)+L"\"";
        status=RegSetValueExW(key,product,0,REG_SZ,reinterpret_cast<const BYTE*>(command.c_str()),static_cast<DWORD>((command.size()+1)*sizeof(wchar_t)));
    } else {
        status=RegDeleteValueW(key,product);if(status==ERROR_FILE_NOT_FOUND)status=ERROR_SUCCESS;
    }
    RegCloseKey(key);return status==ERROR_SUCCESS;
}
void ensureFolders(){
    CreateDirectoryW(folder.c_str(),nullptr);CreateDirectoryW(logFolder.c_str(),nullptr);
}
void logReading(const ob::Reading& r){
    if(!debug)return;
    ensureFolders();
    std::wstring path=logFolder+L"\\debug-0.log";
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if(GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&data) &&
       (data.nFileSizeHigh || data.nFileSizeLow>1000000-256)){
        DeleteFileW((logFolder+L"\\debug-4.log").c_str());
        for(int i=3;i>=0;--i)MoveFileExW((logFolder+L"\\debug-"+std::to_wstring(i)+L".log").c_str(),
            (logFolder+L"\\debug-"+std::to_wstring(i+1)+L".log").c_str(),MOVEFILE_REPLACE_EXISTING);
    }
    HANDLE file=CreateFileW(path.c_str(),FILE_APPEND_DATA,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return;
    SYSTEMTIME t;GetLocalTime(&t);
    char line[256];
    const char* state=r.state==ob::State::reading?"reading":r.state==ob::State::inactive?"inactive":r.state==ob::State::disconnected?"disconnected":"error";
    int n=snprintf(line,sizeof(line),"%04u-%02u-%02uT%02u:%02u:%02u receiver=3151:402d state=%s percent=%d charging=unknown win32_error=%lu\r\n",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,state,r.percent,r.error);
    DWORD done;WriteFile(file,line,static_cast<DWORD>(n),&done,nullptr);CloseHandle(file);
}
bool darkTheme(){
    DWORD light=1,size=sizeof(light);
    RegGetValueW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",L"SystemUsesLightTheme",
        RRF_RT_REG_DWORD,nullptr,&light,&size);
    return light==0;
}
HICON makeIcon(const wchar_t* text,COLORREF color,bool crossed=false,const wchar_t* save=nullptr){
    int size=GetSystemMetrics(SM_CXSMICON);if(size<16)size=16;
    if(!crossed && text[0] && wcsspn(text,L"0123456789")==wcslen(text)){
        if(HICON numeric=ob::numberIcon(text,color,size,save))return numeric;
    }
    HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=size;info.bmiHeader.biHeight=-size;info.bmiHeader.biPlanes=1;
    info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr;
    HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    auto old=SelectObject(dc,bitmap);
    RECT rect{0,0,size,size};
    HBRUSH background=CreateSolidBrush(RGB(255,0,255));FillRect(dc,&rect,background);DeleteObject(background);
    if(crossed){
        HPEN pen=CreatePen(PS_SOLID,size>20?2:1,color);
        auto oldPen=SelectObject(dc,pen),oldBrush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
        Ellipse(dc,size/4,1,size*3/4,size-1);
        MoveToEx(dc,size/2,2,nullptr);LineTo(dc,size/2,size/2);
        MoveToEx(dc,2,size-2,nullptr);LineTo(dc,size-2,2);
        SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);
    }else{
        int height=(wcslen(text)>=3)?size*3/4:size-1;
        HFONT font=CreateFontW(-height,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);
        DrawTextW(dc,text,-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        SelectObject(dc,oldFont);DeleteObject(font);
    }
    auto p=static_cast<DWORD*>(pixels);
    for(int i=0;i<size*size;++i)p[i]=(p[i]&0x00ffffff)==0x00ff00ff?0:(p[i]|0xff000000);
    if(save){
        BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);
        file.bfSize=file.bfOffBits+size*size*4;
        HANDLE h=CreateFileW(save,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);
        if(h!=INVALID_HANDLE_VALUE){DWORD n;WriteFile(h,&file,sizeof(file),&n,nullptr);
            WriteFile(h,&info.bmiHeader,sizeof(BITMAPINFOHEADER),&n,nullptr);WriteFile(h,p,size*size*4,&n,nullptr);CloseHandle(h);}
    }
    HBITMAP mask=CreateBitmap(size,size,1,1,nullptr);
    ICONINFO ii{};ii.fIcon=TRUE;ii.hbmColor=bitmap;ii.hbmMask=mask;
    HICON result=CreateIconIndirect(&ii);
    DeleteObject(mask);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(nullptr,screen);
    return result;
}
std::wstring timeText(){
    if(lastPercent<0)return L"";
    wchar_t b[80];swprintf(b,80,L"%04u-%02u-%02u %02u:%02u:%02u",lastTime.wYear,lastTime.wMonth,lastTime.wDay,lastTime.wHour,lastTime.wMinute,lastTime.wSecond);
    return b;
}
std::wstring description(){
    std::wstring s=L"AJ179 Pro — ";
    if(current.state==ob::State::reading)s+=std::to_wstring(current.percent)+L"%";
    else if(current.state==ob::State::inactive)s+=L"Mouse inactive";
    else if(current.state==ob::State::disconnected)s+=L"Receiver disconnected";
    else s+=lastPercent<0?L"No reading":L"Could not read battery";
    if(lastPercent>=0){
        s+=current.state==ob::State::reading?L"\nRead: ":L"\nLast: "+std::to_wstring(lastPercent)+L"% at ";
        s+=timeText();
    }
    return s;
}
NOTIFYICONDATAW trayData(){
    NOTIFYICONDATAW d{};d.cbSize=sizeof(d);d.hWnd=window;d.uID=1;return d;
}
void updateIcon(bool add=false){
    wchar_t text[8]=L"!";
    COLORREF color=RGB(145,145,145);bool crossed=false;
    if(current.state==ob::State::reading){
        swprintf(text,8,L"%d",current.percent);
        color=ob::batteryColor(current.percent,darkTheme());
    } else if(current.state==ob::State::inactive)wcscpy(text,L"Zzz");
    else if(current.state==ob::State::disconnected)crossed=true;
    HICON next=nullptr;
    if(current.state!=ob::State::reading){
        auto artwork=current.state==ob::State::inactive?ob::Artwork::sleep:
            current.state==ob::State::disconnected?ob::Artwork::disconnected:ob::Artwork::error;
        next=ob::loadArtwork(artwork,GetSystemMetrics(SM_CXSMICON),darkTheme());
    }
    if(!next)next=makeIcon(text,color,crossed);
    auto d=trayData();d.uFlags=NIF_ICON|NIF_TIP|NIF_MESSAGE|NIF_SHOWTIP;d.hIcon=next;d.uCallbackMessage=trayMessage;
    auto tip=description();wcsncpy(d.szTip,tip.c_str(),127);d.szTip[127]=0;
    Shell_NotifyIconW(add?NIM_ADD:NIM_MODIFY,&d);
    if(add){d.uVersion=NOTIFYICON_VERSION_4;Shell_NotifyIconW(NIM_SETVERSION,&d);}
    if(icon)DestroyIcon(icon);icon=next;
}
void showWarning(){
    auto d=trayData();d.uFlags=NIF_INFO;d.dwInfoFlags=NIIF_WARNING|NIIF_RESPECT_QUIET_TIME;
    wcscpy(d.szInfoTitle,L"Mouse battery low");
    swprintf(d.szInfo,256,L"AJ179 Pro battery: %d%%. Please charge your mouse.",current.percent);
    Shell_NotifyIconW(NIM_MODIFY,&d);
}
DWORD WINAPI pollWorker(void*){
    auto r=new ob::Reading(ob::query());
    if(!PostMessageW(window,resultMessage,0,reinterpret_cast<LPARAM>(r)))delete r;
    return 0;
}
void refresh(){
    if(worker)return;
    worker=CreateThread(nullptr,0,pollWorker,nullptr,0,nullptr);
}
void copyDiagnostics(){
    std::wstring text=L"Open Battery " OB_VERSION L"\r\nReceiver: 3151:402D\r\n"+description()+
        L"\r\nCharging: unknown\r\nWin32 error: "+std::to_wstring(current.error)+
        L"\r\nA receiver access error may be caused by another HID application.\r\n";
    if(!OpenClipboard(window))return;
    HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,(text.size()+1)*sizeof(wchar_t));
    if(mem){void* p=GlobalLock(mem);if(p){
        memcpy(p,text.c_str(),(text.size()+1)*sizeof(wchar_t));GlobalUnlock(mem);
        EmptyClipboard();if(!SetClipboardData(CF_UNICODETEXT,mem))GlobalFree(mem);
    }else GlobalFree(mem);}
    CloseClipboard();
}
void menu(){
    HMENU m=CreatePopupMenu();
    AppendMenuW(m,MF_STRING,refreshId,L"Refresh now");
    AppendMenuW(m,MF_STRING|(autoStart()?MF_CHECKED:0),autoId,L"Start with Windows");
    AppendMenuW(m,MF_SEPARATOR,0,nullptr);
    AppendMenuW(m,MF_STRING,copyId,L"Copy diagnostics");
    AppendMenuW(m,MF_STRING|(debug?MF_CHECKED:0),debugId,L"Debug mode");
    AppendMenuW(m,MF_STRING,logsId,L"Open logs folder");
    AppendMenuW(m,MF_SEPARATOR,0,nullptr);AppendMenuW(m,MF_STRING,exitId,L"Exit");
    POINT p;GetCursorPos(&p);SetForegroundWindow(window);
    UINT cmd=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,window,nullptr);
    DestroyMenu(m);PostMessageW(window,WM_NULL,0,0);
    switch(cmd){
    case refreshId:refresh();break;
    case autoId:if(!setAutoStart(!autoStart()))MessageBoxW(window,L"Could not update Start with Windows.",product,MB_OK|MB_ICONERROR);break;
    case copyId:copyDiagnostics();break;
    case debugId:debug=!debug;writeSetting(L"Debug",debug);if(debug)logReading(current);break;
    case logsId:ensureFolders();ShellExecuteW(window,L"open",logFolder.c_str(),nullptr,nullptr,SW_SHOWNORMAL);break;
    case exitId:DestroyWindow(window);break;
    }
}
LRESULT CALLBACK wndProc(HWND h,UINT msg,WPARAM w,LPARAM l){
    if(msg==taskbarCreated && taskbarCreated){updateIcon(true);return 0;}
    switch(msg){
    case WM_TIMER:refresh();return 0;
    case WM_DEVICECHANGE:refresh();return 0;
    case WM_POWERBROADCAST:if(w==PBT_APMRESUMEAUTOMATIC)refresh();return TRUE;
    case WM_SETTINGCHANGE:case WM_THEMECHANGED:updateIcon();return 0;
    case resultMessage:{
        auto r=reinterpret_cast<ob::Reading*>(l);current=*r;delete r;
        if(worker){WaitForSingleObject(worker,INFINITE);CloseHandle(worker);worker=nullptr;}
        if(current.state==ob::State::reading){lastPercent=current.percent;GetLocalTime(&lastTime);}
        bool before=alert.warned;bool warn=alert.observe(current);
        if(before!=alert.warned)writeSetting(L"Warned",alert.warned);
        logReading(current);updateIcon();if(warn)showWarning();return 0;
    }
    case trayMessage:{
        UINT event=LOWORD(l);
        if(event==WM_CONTEXTMENU || event==NIN_KEYSELECT)menu();
        else if(event==WM_LBUTTONUP || event==NIN_SELECT)refresh();
        return 0;
    }
    case WM_CLOSE:DestroyWindow(h);return 0;
    case WM_DESTROY:{
        KillTimer(h,1);
        auto d=trayData();Shell_NotifyIconW(NIM_DELETE,&d);
        if(worker){CancelSynchronousIo(worker);WaitForSingleObject(worker,2000);CloseHandle(worker);worker=nullptr;}
        if(icon){DestroyIcon(icon);icon=nullptr;}
        PostQuitMessage(0);return 0;
    }}
    return DefWindowProcW(h,msg,w,l);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    int argc=0;wchar_t** argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(argc>1 && std::wstring(argv[1])==L"--render-icons"){
        auto a=makeIcon(L"87",RGB(255,255,255),false,L"build\\icon-normal.bmp");DestroyIcon(a);
        a=makeIcon(L"100",RGB(255,255,255),false,L"build\\icon-100.bmp");DestroyIcon(a);
        a=makeIcon(L"20",RGB(235,145,25),false,L"build\\icon-low.bmp");DestroyIcon(a);
        a=makeIcon(L"Zzz",RGB(145,145,145),false,L"build\\icon-sleep.bmp");DestroyIcon(a);
        a=makeIcon(L"!",RGB(145,145,145),false,L"build\\icon-error.bmp");DestroyIcon(a);
        a=makeIcon(L"",RGB(145,145,145),true,L"build\\icon-disconnected.bmp");DestroyIcon(a);
        LocalFree(argv);return 0;
    }
    if(argc>1 && std::wstring(argv[1])==L"--uninstall-cleanup"){
        HWND existing=FindWindowW(L"OpenBattery.Tray",product);
        if(existing){DWORD_PTR result;SendMessageTimeoutW(existing,WM_CLOSE,0,0,SMTO_ABORTIFHUNG,5000,&result);}
        PWSTR localPath=nullptr;
        if(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&localPath))){
            std::wstring root=std::wstring(localPath)+L"\\Open Battery";
            CoTaskMemFree(localPath);
            for(int i=0;i<5;++i)DeleteFileW((root+L"\\Logs\\debug-"+std::to_wstring(i)+L".log").c_str());
            RemoveDirectoryW((root+L"\\Logs").c_str());RemoveDirectoryW(root.c_str());
        }
        setAutoStart(false);RegDeleteTreeW(HKEY_CURRENT_USER,settingsKey);
        LocalFree(argv);return 0;
    }

    LocalFree(argv);
    HANDLE singleton=CreateMutexW(nullptr,FALSE,L"Local\\OpenBattery.Tray");
    if(!singleton || GetLastError()==ERROR_ALREADY_EXISTS){if(singleton)CloseHandle(singleton);return 0;}
    PWSTR local=nullptr;
    if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&local))){CloseHandle(singleton);return 1;}
    folder=std::wstring(local)+L"\\Open Battery";CoTaskMemFree(local);logFolder=folder+L"\\Logs";
    alert.warned=readSetting(L"Warned")!=0;debug=readSetting(L"Debug")!=0;
    initialized=readSetting(L"Initialized")!=0;
    if(!initialized){if(setAutoStart(true))writeSetting(L"Initialized",1);}
    HICON appIcon=ob::loadArtwork(ob::Artwork::app,GetSystemMetrics(SM_CXICON),true);
    WNDCLASSW wc{};wc.hIcon=appIcon;wc.lpfnWndProc=wndProc;wc.hInstance=instance;wc.lpszClassName=L"OpenBattery.Tray";
    if(!RegisterClassW(&wc)){CloseHandle(singleton);return 2;}
    window=CreateWindowExW(WS_EX_TOOLWINDOW,wc.lpszClassName,product,WS_OVERLAPPED,0,0,0,0,nullptr,nullptr,instance,nullptr);
    if(!window){CloseHandle(singleton);return 3;}
    taskbarCreated=RegisterWindowMessageW(L"TaskbarCreated");
    updateIcon(true);SetTimer(window,1,60000,nullptr);refresh();
    MSG msg;
    while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}
    UnregisterClassW(wc.lpszClassName,instance);
    if(appIcon)DestroyIcon(appIcon);
    ob::unloadNumberFont();
    CloseHandle(singleton);return 0;
}
