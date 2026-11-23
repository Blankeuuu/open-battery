#pragma once
#include <windows.h>
#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>
namespace ob {
inline bool targetMouseName(std::wstring name){
    std::transform(name.begin(),name.end(),name.begin(),[](wchar_t c){return std::towlower(c);});
    return name.find(L"vid_3151&pid_402d")!=std::wstring::npos;
}
// A one-shot, process-local subscription. It is absent while normal readings
// are available. Only the device header is inspected: no positions/buttons.
class WakeInput {
    bool registered=false;
    std::vector<HANDLE> targets;
public:
    DWORD error=0;
    bool arm(HWND window){
        if(registered)return true;
        UINT count=0;
        if(GetRawInputDeviceList(nullptr,&count,sizeof(RAWINPUTDEVICELIST))==UINT(-1)){error=GetLastError();return false;}
        std::vector<RAWINPUTDEVICELIST> devices(count);
        if(count && GetRawInputDeviceList(devices.data(),&count,sizeof(RAWINPUTDEVICELIST))==UINT(-1)){error=GetLastError();return false;}
        targets.clear();
        for(UINT i=0;i<count;++i){
            if(devices[i].dwType!=RIM_TYPEMOUSE)continue;
            UINT chars=0;
            if(GetRawInputDeviceInfoW(devices[i].hDevice,RIDI_DEVICENAME,nullptr,&chars)==UINT(-1))continue;
            std::vector<wchar_t> name(chars+1,0);
            if(GetRawInputDeviceInfoW(devices[i].hDevice,RIDI_DEVICENAME,name.data(),&chars)==UINT(-1))continue;
            if(targetMouseName(name.data()))targets.push_back(devices[i].hDevice);
        }
        if(targets.empty())return false;
        RAWINPUTDEVICE device{1,2,RIDEV_INPUTSINK,window};
        registered=RegisterRawInputDevices(&device,1,sizeof(device))!=FALSE;
        error=registered?0:GetLastError();
        if(!registered)targets.clear();
        return registered;
    }
    bool accepts(HRAWINPUT input)const{
        if(!registered)return false;
        RAWINPUTHEADER header{};UINT size=sizeof(header);
        if(GetRawInputData(input,RID_HEADER,&header,&size,sizeof(header))!=sizeof(header))return false;
        return header.dwType==RIM_TYPEMOUSE &&
            std::find(targets.begin(),targets.end(),header.hDevice)!=targets.end();
    }
    void stop(){
        if(registered){RAWINPUTDEVICE device{1,2,RIDEV_REMOVE,nullptr};
            if(!RegisterRawInputDevices(&device,1,sizeof(device)))error=GetLastError();}
        registered=false;targets.clear();
    }
    bool armed()const{return registered;}
};
}
