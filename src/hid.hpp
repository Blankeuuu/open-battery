#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <setupapi.h>
extern "C" {
#include <hidsdi.h>
#include <hidpi.h>
}
#include <vector>
#include <string>
#include "model.hpp"

namespace ob {
inline Reading query() {
    Reading result;
    GUID guid; HidD_GetHidGuid(&guid);
    HDEVINFO list=SetupDiGetClassDevsW(&guid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(list==INVALID_HANDLE_VALUE){result.error=GetLastError();return result;}
    bool present=false, control=false;
    for(DWORD i=0;;++i) {
        SP_DEVICE_INTERFACE_DATA iface{}; iface.cbSize=sizeof(iface);
        if(!SetupDiEnumDeviceInterfaces(list,nullptr,&guid,i,&iface)) break;
        DWORD size=0; SetupDiGetDeviceInterfaceDetailW(list,&iface,nullptr,0,&size,nullptr);
        if(!size) continue;
        std::vector<BYTE> storage(size);
        auto detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());
        detail->cbSize=sizeof(*detail);
        if(!SetupDiGetDeviceInterfaceDetailW(list,&iface,detail,size,nullptr,nullptr)) continue;
        std::wstring path=detail->DevicePath;
        if(path.find(L"vid_3151&pid_402d")==std::wstring::npos) continue;
        present=true;
        HANDLE h=CreateFileW(path.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
        if(h==INVALID_HANDLE_VALUE){result.error=GetLastError();continue;}
        PHIDP_PREPARSED_DATA pp=nullptr; HIDP_CAPS caps{};
        if(HidD_GetPreparsedData(h,&pp)){HidP_GetCaps(pp,&caps);HidD_FreePreparsedData(pp);}
        CloseHandle(h);
        // Never open the OS mouse/keyboard streams. Only the vendor control collection.
        if(caps.UsagePage!=0xffff || caps.Usage!=2 || caps.FeatureReportByteLength!=65) continue;
        control=true;
        h=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
        if(h==INVALID_HANDLE_VALUE){result.error=GetLastError();continue;}
        BYTE request[65]{}; request[1]=0xf7; // report ID 0, status poll only
        if(!HidD_SetFeature(h,request,sizeof(request))) result.error=GetLastError();
        else {
            Sleep(50); // Receiver round trip; no persistent streaming/polling.
            BYTE response[65]{};
            if(HidD_GetFeature(h,response,sizeof(response))) {
                result=decode(response,sizeof(response));
                if(result.state==State::error) result.error=ERROR_INVALID_DATA;
            } else result.error=GetLastError();
        }
        CloseHandle(h);
        break; // One paired receiver is in scope for v0.1.
    }
    SetupDiDestroyDeviceInfoList(list);
    if(!present) result.state=State::disconnected;
    else if(!control && !result.error) result.error=ERROR_NOT_SUPPORTED;
    return result;
}
}
