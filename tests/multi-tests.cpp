#define OB_OFFSCREEN_TEST
#define wWinMain unusedAppEntry
#include "../src/main.cpp"
#undef wWinMain
#include <cstdlib>
#include <psapi.h>
void requireMulti(bool ok,const char* name){if(!ok){printf("FAIL: %s\n",name);exit(1);}}
void captureFixture(HWND h){
    RECT bounds;GetClientRect(h,&bounds);int width=bounds.right,height=bounds.bottom;
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    void* pixels=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&pixels,nullptr,0);auto old=SelectObject(dc,bitmap);
    RECT r{0,0,width,height};FillRect(dc,&r,reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
    SendMessageW(h,WM_PRINT,reinterpret_cast<WPARAM>(dc),PRF_CLIENT|PRF_CHILDREN|PRF_ERASEBKGND);
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+width*height*4;
    HANDLE file=CreateFileW(L"build\\devices-fixture.bmp",GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);DWORD n;
    if(file!=INVALID_HANDLE_VALUE){WriteFile(file,&header,sizeof(header),&n,nullptr);WriteFile(file,&bi.bmiHeader,sizeof(bi.bmiHeader),&n,nullptr);WriteFile(file,pixels,width*height*4,&n,nullptr);CloseHandle(file);}
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
}
void messages(){MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE))if(m.message!=WM_QUIT){TranslateMessage(&m);DispatchMessageW(&m);}}
int main(int argc,char**){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    ob::deviceSettingsRoot=L"Software\\Open Battery Tests\\Multi-"+std::to_wstring(GetCurrentProcessId());
    ob::Device mouse;mouse.id=L"aj179";mouse.defaultName=L"AJ179 Pro";mouse.kind=ob::DeviceKind::mouse;mouse.connection=L"2.4 GHz";mouse.enabled=true;mouse.notifications=false;mouse.observe({ob::State::reading,100},fileTimeNow());
    ob::Device pad;pad.id=L"fixture-pad";pad.defaultName=L"DualShock 4";pad.kind=ob::DeviceKind::controller;pad.connection=L"Bluetooth";pad.enabled=true;pad.notifications=false;pad.badge=2;pad.observe({ob::State::reading,55,true,0,true,true},fileTimeNow());
    ob::Device headphones;headphones.id=L"fixture-jbl";headphones.defaultName=L"JBL TUNE700BT";headphones.kind=ob::DeviceKind::headphones;headphones.connection=L"Bluetooth";headphones.enabled=true;headphones.notifications=false;headphones.observe({ob::State::reading,18},fileTimeNow());
    devices={mouse,pad,headphones};current=mouse.reading;
    pad.alias=L"My pad";pad.alert.warned=true;pad.extendedReports=true;pad.ringColor=RGB(170,80,245);pad.customRingColor=true;ob::saveDevice(pad);ob::Device restored;restored.id=pad.id;ob::loadDevice(restored);
    requireMulti(restored.alias==L"My pad" && restored.enabled && !restored.notifications && restored.badge==2 && restored.alert.warned && restored.extendedReports && restored.identityColor()==RGB(170,80,245),"per-device preferences and warning survive reload");
    ob::forgetDevice(restored);ob::Device forgotten;forgotten.id=pad.id;ob::loadDevice(forgotten);requireMulti(!forgotten.configured && !forgotten.enabled && forgotten.alias.empty(),"forget clears only app preferences");
    requireMulti(ob::rememberedDevices().empty(),"forgotten device removed from persisted list");
    for(auto& d:devices)ob::saveDevice(d);
    requireMulti(ob::rememberedDevices().size()==3,"three identities persist independently");
    WNDCLASSW cls{};cls.lpfnWndProc=wndProc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"OpenBattery.MultiTest";RegisterClassW(&cls);
    window=CreateWindowExW(0,cls.lpszClassName,L"Multi fixture",0,0,0,0,0,nullptr,nullptr,cls.hInstance,nullptr);
    multiMode=true;updateAllIcons();requireMulti(trays.size()==3,"one colored ring per selected device");
    auto first=trays[0].number;auto cached=trays[0].icon;updateAllIcons();requireMulti(trays[0].icon==cached,"unchanged ring uses cached icon");for(auto& t:trays){auto data=deviceTrayData(t.number);Shell_NotifyIconW(NIM_DELETE,&data);}updateAllIcons(true);requireMulti(trays.size()==3 && trays[0].number==first,"Explorer rebuild preserves tray mapping");
    for(auto& d:devices)d.enabled=false;updateAllIcons();requireMulti(trays.size()==1 && trays[0].id==L"app","fallback app icon when none selected");
    for(auto& d:devices)d.enabled=true;updateAllIcons();
    showDevices(pad.id);requireMulti(devicesWindow && selectedDeviceId()==pad.id,"tray target selects correct device");
    captureFixture(devicesWindow);
    wchar_t detail[1024]{};GetWindowTextW(detailsLabel,detail,1024);requireMulti(std::wstring(detail).find(L"Approx.")!=std::wstring::npos && std::wstring(detail).find(L"Charging")!=std::wstring::npos,"approximation and charging in details");
    SetWindowTextW(nameEdit,L"Living room pad");SetWindowTextW(badgeEdit,L"3");SendMessageW(devicesWindow,WM_COMMAND,saveId,0);
    requireMulti(devices[1].alias==L"Living room pad" && devices[1].badge==3,"save name and number updates correct device");
    SendMessageW(notifyCheck,BM_SETCHECK,BST_CHECKED,0);SendMessageW(devicesWindow,WM_COMMAND,notifyId,0);requireMulti(devices[1].blinkLow,"blink checkbox targets one device");
    SendMessageW(enabledCheck,BM_SETCHECK,BST_UNCHECKED,0);SendMessageW(devicesWindow,WM_COMMAND,enabledId,0);requireMulti(!devices[1].enabled && trays.size()==2,"disable hides only the selected device ring");
    devices[1].enabled=true;updateAllIcons();refreshDeviceWindow();
    ob::Device test=mouse;HICON warm=ob::deviceIcon(test,16,true);DestroyIcon(warm);
    DWORD before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    for(int i=0;i<1000;++i){test.kind=ob::DeviceKind(i%3);test.reading.state=ob::State(i%5);test.badge=i%4;HICON icon=ob::ringIcon(test,16+(i%3)*4,i%2);requireMulti(icon!=nullptr,"A device icon creation");DestroyIcon(icon);}
    DWORD after=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);requireMulti(after<=before+2,"new A renderer has stable GDI handles");
        DestroyWindow(devicesWindow);
    for(int i=0;i<10;++i){showDevices(pad.id);DestroyWindow(devicesWindow);}
    PROCESS_MEMORY_COUNTERS_EX memoryBefore{};memoryBefore.cb=sizeof(memoryBefore);
    GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memoryBefore),sizeof(memoryBefore));
    DWORD uiGdiBefore=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),uiHandlesBefore=0;GetProcessHandleCount(GetCurrentProcess(),&uiHandlesBefore);
    for(int i=0;i<100;++i){showDevices(pad.id);DestroyWindow(devicesWindow);}
    PROCESS_MEMORY_COUNTERS_EX memoryAfter{};memoryAfter.cb=sizeof(memoryAfter);
    GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memoryAfter),sizeof(memoryAfter));
    DWORD uiGdiAfter=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),uiHandlesAfter=0;GetProcessHandleCount(GetCurrentProcess(),&uiHandlesAfter);
    requireMulti(uiGdiAfter<=uiGdiBefore+2 && uiHandlesAfter<=uiHandlesBefore+2,"100 Devices open/close cycles release fonts and handles");
    requireMulti(memoryAfter.PrivateUsage<=memoryBefore.PrivateUsage+1000000,"100 Devices cycles have bounded private memory after warmup");
    printf("PASS: 100 Devices cycles private=%zu -> %zu GDI=%lu -> %lu handles=%lu -> %lu\n",memoryBefore.PrivateUsage,memoryAfter.PrivateUsage,uiGdiBefore,uiGdiAfter,uiHandlesBefore,uiHandlesAfter);
    printf("PASS: settings reload/forget, one colored ring per device, fallback, Explorer rebuild, Devices selection/editing/toggles, 1000 ring icons GDI %lu -> %lu\n",before,after);
    if(argc>1){ULONGLONG end=GetTickCount64()+45000;while(devicesWindow && GetTickCount64()<end){messages();Sleep(10);}}
    DestroyWindow(window);RegDeleteTreeW(HKEY_CURRENT_USER,ob::deviceSettingsRoot.c_str());ob::unloadNumberFont();
}
