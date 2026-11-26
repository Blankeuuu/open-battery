#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <map>
#include <cstdio>
#include <cstdlib>
static std::map<UINT,HICON> shown;
static unsigned popups=0,updates=0,queries=0;
static BOOL testNotify(DWORD command,PNOTIFYICONDATAW data){
    if(data->uFlags&NIF_INFO)++popups;
    if((command==NIM_ADD || command==NIM_MODIFY) && (data->uFlags&NIF_ICON)){shown[data->uID]=data->hIcon;++updates;}
    if(command==NIM_DELETE)shown.erase(data->uID);return TRUE;
}
#define Shell_NotifyIconW testNotify
#define OB_OFFSCREEN_TEST
#define wWinMain unusedEntry
#include "../src/main.cpp"
#undef wWinMain
#undef Shell_NotifyIconW
static void requireBlink(bool ok,const char* text){if(!ok){printf("FAIL: %s\n",text);exit(1);}}
static ob::Reading countedQuery(){++queries;return {ob::State::reading,19};}
static LRESULT CALLBACK fixtureProc(HWND h,UINT m,WPARAM w,LPARAM l){if(m==WM_DEVICECHANGE)return 0;return wndProc(h,m,w,l);}
static std::vector<DWORD> pixels(HICON icon){
    ICONINFO ii{};GetIconInfo(icon,&ii);BITMAP bm{};GetObjectW(ii.hbmColor,sizeof(bm),&bm);int size=bm.bmWidth;
    BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=size;bi.bmiHeader.biHeight=-size;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    std::vector<DWORD> data(size*size);HDC dc=CreateCompatibleDC(nullptr);GetDIBits(dc,ii.hbmColor,0,size,data.data(),&bi,DIB_RGB_COLORS);DeleteDC(dc);DeleteObject(ii.hbmColor);DeleteObject(ii.hbmMask);return data;
}
int main(){
    ob::deviceSettingsRoot=L"Software\\Open Battery Tests\\Blink-"+std::to_wstring(GetCurrentProcessId());
    WNDCLASSW c{};c.hInstance=GetModuleHandleW(nullptr);c.lpfnWndProc=fixtureProc;c.lpszClassName=L"OpenBattery.BlinkTest";RegisterClassW(&c);
    window=CreateWindowExW(0,c.lpszClassName,L"Blink fixture",0,0,0,0,0,nullptr,nullptr,c.hInstance,nullptr);
    queryBattery=countedQuery;multiMode=true;
    ob::Device low;low.id=L"fixture-low";low.defaultName=L"Mouse";low.kind=ob::DeviceKind::mouse;low.enabled=true;low.notifications=false;low.alert.warned=true;low.reading={ob::State::reading,19};
    ob::Device healthy=low;healthy.id=L"fixture-healthy";healthy.reading.percent=80;
    devices={low,healthy};updateAllIcons();
    UINT lowId=trays[0].number,healthyId=trays[1].number;HICON bright=shown[lowId],other=shown[healthyId];auto brightPixels=pixels(bright);
    unsigned beforeUpdates=updates;ULONGLONG deadline=GetTickCount64()+1600;
    while(shown[lowId]==bright && GetTickCount64()<deadline){MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}Sleep(1);}
    requireBlink(shown[lowId]!=bright,"19% blinks even with an old Warned flag and legacy notifications disabled");
    requireBlink(shown[healthyId]==other && updates==beforeUpdates+1,"only the low device is updated on a blink tick");
    auto dimPixels=pixels(shown[lowId]);bool differs=false;int size=int(std::sqrt(double(dimPixels.size())));
    for(int y=0;y<size;++y)for(int x=0;x<size;++x){if(brightPixels[y*size+x]!=dimPixels[y*size+x])differs=true;
        if(x>=size*5/16 && x<size*11/16 && y>=size*5/16 && y<size*11/16)requireBlink(brightPixels[y*size+x]==dimPixels[y*size+x],"central percentage never blinks");}
    requireBlink(differs,"the ring really changes pixels");
    SendMessageW(window,WM_TIMER,5,0);requireBlink(shown[lowId]==bright,"second tick restores the cached bright frame");
    DWORD gdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    for(int i=0;i<1000;++i)SendMessageW(window,WM_TIMER,5,0);
    requireBlink(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)==gdi,"1000 blink ticks allocate no additional GDI objects");
    requireBlink(queries==0 && worker==nullptr,"blinking never polls HID or creates a worker");
    devices[0].reading.percent=21;updateAllIcons();HICON stopped=shown[lowId];beforeUpdates=updates;SendMessageW(window,WM_TIMER,5,0);
    requireBlink(shown[lowId]==stopped && updates==beforeUpdates,"21% stops timer effects, including a queued old tick");
    devices[0].reading.percent=20;updateAllIcons();bright=shown[lowId];SendMessageW(window,WM_TIMER,5,0);requireBlink(shown[lowId]!=bright,"20% is included in the threshold");
    devices[0].blinkLow=false;updateAllIcons();stopped=shown[lowId];beforeUpdates=updates;SendMessageW(window,WM_TIMER,5,0);requireBlink(shown[lowId]==stopped && updates==beforeUpdates,"per-device opt-out stops blinking");
    ob::saveDevice(devices[0]);ob::Device restored;restored.id=devices[0].id;ob::loadDevice(restored);requireBlink(!restored.blinkLow,"blink opt-out survives reload");ob::forgetDevice(restored);requireBlink(restored.blinkLow,"forget restores blink default");
    devices[0].blinkLow=true;updateAllIcons();
    devices[0].reading.chargingKnown=true;devices[0].reading.charging=true;updateAllIcons();stopped=shown[lowId];SendMessageW(window,WM_TIMER,5,0);requireBlink(shown[lowId]==stopped,"confirmed charging stops blinking");
    for(auto state:{ob::State::inactive,ob::State::disconnected,ob::State::unsupported,ob::State::error}){
        devices[0].reading={state,-1};updateAllIcons();stopped=shown[lowId];beforeUpdates=updates;SendMessageW(window,WM_TIMER,5,0);requireBlink(shown[lowId]==stopped && updates==beforeUpdates,"historical or unavailable readings never blink");
    }
    devices[0].notifications=true;devices[0].alert.warned=false;devices[0].reading={ob::State::reading,10};updateAllIcons();SendMessageW(window,WM_TIMER,5,0);requireBlink(popups==0,"low battery never requests a Windows notification");
    DestroyWindow(window);RegDeleteTreeW(HKEY_CURRENT_USER,ob::deviceSettingsRoot.c_str());ob::unloadNumberFont();
    puts("PASS: threshold 20/21, cached ring blink, unchanged digits, no popups/HID/GDI growth, charging and invalid-state stop");
}
