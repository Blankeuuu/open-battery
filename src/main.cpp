#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <dbt.h>
#include <commdlg.h>
#include <shlobj.h>
#include <cstdio>
#include <string>
#include <vector>
#include "hid.hpp"
#include "icons.hpp"
#include "wake-input.hpp"
#include "number-icon.hpp"
#include "bluetooth.hpp"
#include "device-settings.hpp"
#include "device-icon.hpp"
#include "ring-icon.hpp"
#include "../build/version.hpp"

namespace {
constexpr wchar_t product[]=L"Open Battery";
constexpr wchar_t settingsKey[]=L"Software\\Open Battery";
constexpr wchar_t runKey[]=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr UINT trayMessage=WM_APP+1, resultMessage=WM_APP+2, wakeMessage=WM_APP+3;
constexpr UINT refreshId=100, autoId=101, copyId=102, debugId=103, logsId=104, exitId=105;
HWND window=nullptr;
HANDLE worker=nullptr;
ob::WakeInput wakeInput;
bool wakeRequested=false;
unsigned wakeAttempts=0,wakeTriggers=0;
ULONGLONG nextWakeAllowed=0,wakeStartedTick=0,lastWakeLatencyMs=0;
HICON icon=nullptr;
UINT taskbarCreated=0;
HDEVNOTIFY deviceNotification=nullptr;
bool debug=false, initialized=false;
bool multiMode=false;
void updateAllIcons(bool add);
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
void logReading(const ob::Reading& r,const char* label="3151:402d"){
    if(!debug)return;
    ensureFolders();
    std::wstring path=logFolder+L"\\debug-0.log";
    WIN32_FILE_ATTRIBUTE_DATA data{};
    if(GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&data) &&
       (data.nFileSizeHigh || data.nFileSizeLow>1000000-320)){
        DeleteFileW((logFolder+L"\\debug-4.log").c_str());
        for(int i=3;i>=0;--i)MoveFileExW((logFolder+L"\\debug-"+std::to_wstring(i)+L".log").c_str(),
            (logFolder+L"\\debug-"+std::to_wstring(i+1)+L".log").c_str(),MOVEFILE_REPLACE_EXISTING);
    }
    HANDLE file=CreateFileW(path.c_str(),FILE_APPEND_DATA,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return;
    SYSTEMTIME t;GetLocalTime(&t);
    char line[320];
    const char* state=r.state==ob::State::reading?"reading":r.state==ob::State::inactive?"inactive":r.state==ob::State::disconnected?"disconnected":"error";
    int n=snprintf(line,sizeof(line),"%04u-%02u-%02uT%02u:%02u:%02u device=%s state=%s percent=%d charging=%s approximate=%d win32_error=%lu wake=%u wake_ms=%llu\r\n",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,label,state,r.percent,r.chargingKnown?(r.charging?"yes":"no"):"unknown",r.approximate,r.error,wakeTriggers,wakeRequested?GetTickCount64()-wakeStartedTick:0);
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
    if(multiMode){updateAllIcons(add);return;}
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
#include "multi-app.hpp"
ob::Reading pollPrevious;
ob::Reading (*queryBattery)()=ob::query;
DWORD WINAPI pollWorker(void*){
    auto r=new ob::Reading(pollAj?queryBattery():pollPrevious);
    if(multiMode && pollAux){auxResult=ob::queryOtherDevices(pollSelected,false,pollExtended);auxPending=true;}
    if(!PostMessageW(window,resultMessage,0,reinterpret_cast<LPARAM>(r)))delete r;
    return 0;
}
void refresh(){
    if(worker){if(multiMode)refreshQueued=true;return;}
    if(multiMode){
        pollAux=requestedDevice!=L"aj179";pollAj=requestedDevice.empty() || requestedDevice==L"aj179";pollPrevious=current;
        pollSelected.clear();pollExtended.clear();for(const auto& d:devices)if(d.enabled && (requestedDevice.empty() || requestedDevice==d.id)){pollSelected.push_back(d.id);if(d.extendedReports)pollExtended.push_back(d.id);}
        if(auto aj=getDevice(L"aj179");aj && !aj->enabled)pollAj=false;
        requestedDevice.clear();
    }
    worker=CreateThread(nullptr,0,pollWorker,nullptr,0,nullptr);
}
void monitorWake(){
    if(current.state==ob::State::reading || current.state==ob::State::disconnected || wakeRequested){
        wakeInput.stop();return;
    }
    ULONGLONG now=GetTickCount64();
    if(now<nextWakeAllowed){
        SetTimer(window,3,static_cast<UINT>(nextWakeAllowed-now),nullptr);return;
    }
    if(multiMode){auto aj=getDevice(L"aj179");if(!aj || !aj->enabled){wakeInput.stop();return;}}
    wakeInput.arm(window);
}
void requestWakeRefresh(){
    if(current.state==ob::State::reading || current.state==ob::State::disconnected ||
       wakeRequested || GetTickCount64()<nextWakeAllowed)return;
    wakeInput.stop();KillTimer(window,3);
    wakeRequested=true;wakeAttempts=1;++wakeTriggers;wakeStartedTick=GetTickCount64();
    if(multiMode)requestedDevice=L"aj179";
    if(!worker)refresh(); // An in-flight result schedules the bounded wake retry.
}
void updateWakeAfterReading(){
    if(current.state==ob::State::reading || current.state==ob::State::disconnected){
        if(wakeRequested && current.state==ob::State::reading)lastWakeLatencyMs=GetTickCount64()-wakeStartedTick;
        wakeRequested=false;wakeAttempts=0;nextWakeAllowed=0;
        KillTimer(window,2);KillTimer(window,3);wakeInput.stop();return;
    }
    if(wakeRequested && wakeAttempts<4){
        UINT delay=wakeAttempts==1?250:wakeAttempts==2?750:1500;
        ++wakeAttempts;SetTimer(window,2,delay,nullptr);return;
    }
    if(wakeRequested){
        wakeRequested=false;wakeAttempts=0;nextWakeAllowed=GetTickCount64()+15000;
    }
    monitorWake();
}

void copyDiagnostics(){
    std::wstring text=L"Open Battery " OB_VERSION L"\r\nReceiver: 3151:402D\r\n"+description()+
        L"\r\nWake refreshes: "+std::to_wstring(wakeTriggers)+L"\r\nLast wake refresh ms: "+std::to_wstring(lastWakeLatencyMs)+
        L"\r\nWake input error: "+std::to_wstring(wakeInput.error)+
        L"\r\nCharging: unknown\r\nWin32 error: "+std::to_wstring(current.error)+
        L"\r\nA receiver access error may be caused by another HID application.\r\n";
    if(multiMode){for(const auto& d:devices)text+=L"\r\n"+std::wstring(ob::kindText(d.kind))+L" ["+ob::storageId(d.id)+L"] "+d.connection+L": "+ob::stateText(d.reading.state)+L", level="+std::to_wstring(d.reading.percent)+L", error="+std::to_wstring(d.reading.error);}
    if(!OpenClipboard(window))return;
    HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,(text.size()+1)*sizeof(wchar_t));
    if(mem){void* p=GlobalLock(mem);if(p){
        memcpy(p,text.c_str(),(text.size()+1)*sizeof(wchar_t));GlobalUnlock(mem);
        EmptyClipboard();if(!SetClipboardData(CF_UNICODETEXT,mem))GlobalFree(mem);
    }else GlobalFree(mem);}
    CloseClipboard();
}
void menu(const std::wstring& selected=L"aj179"){
    HMENU m=CreatePopupMenu();
    AppendMenuW(m,MF_STRING,refreshId,L"Refresh this device");
    AppendMenuW(m,MF_STRING,devicesId,L"Devices...");
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
    case refreshId:if(multiMode)refreshOne(selected);else refresh();break;
    case devicesId:showDevices(selected);break;
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
    case WM_TIMER:
        if(w==1)refresh();
        else if(w==2){KillTimer(h,2);if(multiMode)requestedDevice=L"aj179";refresh();}
        else if(w==3){KillTimer(h,3);monitorWake();}
        else if(w==4){KillTimer(h,4);refresh();}
        return 0;
    case WM_INPUT:
        if(wakeInput.accepts(reinterpret_cast<HRAWINPUT>(l))){
            wakeInput.stop();PostMessageW(h,wakeMessage,0,0);
        }
        return DefWindowProcW(h,msg,w,l);
    case wakeMessage:requestWakeRefresh();return 0;
    case WM_DEVICECHANGE:wakeInput.stop();SetTimer(h,4,250,nullptr);return 0;
    case WM_POWERBROADCAST:if(w==PBT_APMRESUMEAUTOMATIC)refresh();return TRUE;
    case WM_SETTINGCHANGE:case WM_THEMECHANGED:updateIcon();return 0;
    case resultMessage:{
        auto r=reinterpret_cast<ob::Reading*>(l);if(!multiMode || pollAj)current=*r;delete r;
        if(worker){WaitForSingleObject(worker,INFINITE);CloseHandle(worker);worker=nullptr;}
        if((!multiMode || pollAj) && current.state==ob::State::reading){lastPercent=current.percent;GetLocalTime(&lastTime);}
        bool before=alert.warned;bool warn=!multiMode && alert.observe(current);
        if(before!=alert.warned)writeSetting(L"Warned",alert.warned);
        if(multiMode){
            if(auto aj=getDevice(L"aj179");aj && pollAj){aj->observe(current,fileTimeNow());}
            receiveOtherDevices();updateAllIcons();
            if(auto aj=getDevice(L"aj179");aj && pollAj)warningFor(*aj);
            refreshDeviceWindow();
        }
        if(!multiMode || pollAj)logReading(current);if(!multiMode)updateIcon();updateWakeAfterReading();if(warn && !multiMode)showWarning();
        if(multiMode && refreshQueued){refreshQueued=false;refresh();}return 0;
    }
    case trayMessage:{
        UINT event=LOWORD(l);
        if(multiMode){auto id=trayDeviceId(HIWORD(l));
            if(event==WM_CONTEXTMENU)menu(id);else if(event==NIN_KEYSELECT || event==WM_LBUTTONUP || event==NIN_SELECT)showDevices(id);
        }else if(event==WM_CONTEXTMENU || event==NIN_KEYSELECT)menu();
        else if(event==WM_LBUTTONUP || event==NIN_SELECT)refresh();
        return 0;
    }
    case WM_APP+4:showDevices();return 0;
    case WM_CLOSE:DestroyWindow(h);return 0;
    case WM_DESTROY:{
        KillTimer(h,1);KillTimer(h,2);KillTimer(h,3);KillTimer(h,4);wakeInput.stop();
        if(deviceNotification){UnregisterDeviceNotification(deviceNotification);deviceNotification=nullptr;}
        auto d=trayData();Shell_NotifyIconW(NIM_DELETE,&d);
        if(multiMode){if(devicesWindow)DestroyWindow(devicesWindow);for(auto& t:trays)removeTray(t);trays.clear();}
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

    bool openDevices=argc>1 && std::wstring(argv[1])==L"--devices";
    LocalFree(argv);
    HANDLE singleton=CreateMutexW(nullptr,FALSE,L"Local\\OpenBattery.Tray");
    if(!singleton || GetLastError()==ERROR_ALREADY_EXISTS){if(openDevices){HWND existing=FindWindowW(L"OpenBattery.Tray",product);if(existing)PostMessageW(existing,WM_APP+4,0,0);}if(singleton)CloseHandle(singleton);return 0;}
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
    multiMode=true;initializeDevices();
    DEV_BROADCAST_DEVICEINTERFACE_W filter{};filter.dbcc_size=sizeof(filter);filter.dbcc_devicetype=DBT_DEVTYP_DEVICEINTERFACE;
    deviceNotification=RegisterDeviceNotificationW(window,&filter,DEVICE_NOTIFY_WINDOW_HANDLE|DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
    taskbarCreated=RegisterWindowMessageW(L"TaskbarCreated");
    updateIcon(true);SetTimer(window,1,60000,nullptr);refresh();
    if(openDevices)showDevices();
    MSG msg;
    while(GetMessageW(&msg,nullptr,0,0)>0){if(devicesWindow && IsDialogMessageW(devicesWindow,&msg))continue;TranslateMessage(&msg);DispatchMessageW(&msg);}
    UnregisterClassW(wc.lpszClassName,instance);
    if(appIcon)DestroyIcon(appIcon);
    ob::unloadNumberFont();
    CloseHandle(singleton);return 0;
}
