#pragma once
#include <windows.h>
#include <bthsdpdef.h>
#include <bluetoothapis.h>
#include <setupapi.h>
#include <initguid.h>
#include <devpkey.h>
#include <cfgmgr32.h>
#include <cwctype>
#include "hid.hpp"
#include "devices.hpp"

namespace ob {
inline std::wstring lower(std::wstring s){for(auto& c:s)c=std::towlower(c);return s;}
inline std::wstring compactSerial(const wchar_t* s){
    std::wstring out;for(;*s;++s)if(iswxdigit(*s))out+=std::towlower(*s);
    return out.size()==12?out:L"";
}
inline std::wstring guidText(const GUID& g){wchar_t s[48];StringFromGUID2(g,s,48);return lower(s);}
inline bool property(HDEVINFO list,SP_DEVINFO_DATA& node,const DEVPROPKEY& key,void* data,DWORD size,DEVPROPTYPE* output=nullptr){
    DEVPROPTYPE type=0;DWORD got=0;
    bool ok=SetupDiGetDevicePropertyW(list,&node,&key,&type,static_cast<BYTE*>(data),size,&got,0)!=FALSE;
    if(output)*output=type;return ok;
}
inline std::wstring nodeName(HDEVINFO list,SP_DEVINFO_DATA& node){
    wchar_t name[512]{};
    if(!property(list,node,DEVPKEY_Device_FriendlyName,name,sizeof(name)))property(list,node,DEVPKEY_Device_DeviceDesc,name,sizeof(name));
    name[511]=0;return name;
}
struct BtNode {std::wstring container,address;int percent=-1;};
inline std::vector<Device> discoverBluetooth(){
    std::vector<Device> found;
    // No inquiry and no connection requests: enumerate Windows' known devices only.
    BLUETOOTH_DEVICE_SEARCH_PARAMS search{};search.dwSize=sizeof(search);
    search.fReturnAuthenticated=TRUE;search.fReturnRemembered=TRUE;search.fReturnConnected=TRUE;
    BLUETOOTH_DEVICE_INFO bt{};bt.dwSize=sizeof(bt);
    HBLUETOOTH_DEVICE_FIND scan=BluetoothFindFirstDevice(&search,&bt);
    if(scan){do{
        wchar_t address[16];swprintf(address,16,L"%012llx",bt.Address.ullLong);
        Device d;d.id=L"bt:"+std::wstring(address);d.defaultName=bt.szName;d.connection=L"Bluetooth";
        unsigned major=bt.ulClassofDevice&0x1f00;
        std::wstring name=lower(d.defaultName);
        d.kind=major==0x400?DeviceKind::headphones:DeviceKind::other;
        if(name.find(L"controller")!=name.npos || name.find(L"dualsense")!=name.npos || name.find(L"dualshock")!=name.npos)d.kind=DeviceKind::controller;
        else if(name.find(L"mouse")!=name.npos)d.kind=DeviceKind::mouse;
        d.reading.state=bt.fConnected?State::unsupported:State::disconnected;
        d.source=L"Windows Bluetooth battery";found.push_back(d);
    }while(BluetoothFindNextDevice(scan,&bt));BluetoothFindDeviceClose(scan);}

    HDEVINFO list=SetupDiGetClassDevsW(nullptr,nullptr,nullptr,DIGCF_ALLCLASSES|DIGCF_PRESENT);
    if(list==INVALID_HANDLE_VALUE)return found;
    std::vector<BtNode> nodes;
    // Battery can live on a Hands-Free service child, not the primary BT node.
    // First build the container map, then join profile properties by container.
    for(DWORD i=0;;++i){
        SP_DEVINFO_DATA node{};node.cbSize=sizeof(node);if(!SetupDiEnumDeviceInfo(list,i,&node))break;
        wchar_t instance[1024]{};if(!SetupDiGetDeviceInstanceIdW(list,&node,instance,1024,nullptr))continue;
        std::wstring id=lower(instance);
        if(id.rfind(L"bth",0)!=0)continue;
        GUID container{};if(!property(list,node,DEVPKEY_Device_ContainerId,&container,sizeof(container)))continue;
        BtNode item;item.container=guidText(container);
        auto p=id.find(L"dev_");if(p!=id.npos && id.size()>=p+16)item.address=id.substr(p+4,12);
        // This HFP property is populated by Windows, not a transmitted command.
        // It is undocumented: validate type/range and fail to '?' if absent.
        static const DEVPROPKEY battery={{0x104ea319,0x6ee2,0x4701,{0xbd,0x47,0x8d,0xdb,0xf4,0x25,0xbb,0xe5}},2};
        DWORD value=0;DEVPROPTYPE type=0;
        if(property(list,node,battery,&value,sizeof(value),&type)){
            if((type==DEVPROP_TYPE_BYTE || type==DEVPROP_TYPE_UINT32) && value<=100)item.percent=int(value);
        }
        nodes.push_back(item);
    }
    SetupDiDestroyDeviceInfoList(list);
    for(auto& d:found){
        std::wstring address=d.id.substr(3),container;
        for(const auto& n:nodes)if(n.address==address){container=n.container;break;}
        if(container.empty())continue;d.container=container;
        for(const auto& n:nodes)if(n.container==container && n.percent>=0 && d.reading.state!=State::disconnected){
            d.reading.state=State::reading;d.reading.percent=n.percent;break;
        }
    }
    return found;
}

inline Reading readSony(HANDLE h,unsigned length,bool sense,bool bt,unsigned featureLength=0,bool allowExtended=false){
    Reading result;result.state=State::unsupported;
    if(length<10 || length>4096)return result;
    std::vector<BYTE> buffer(length,0);BYTE* bytes=buffer.data();OVERLAPPED ov{};ov.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!ov.hEvent){result.state=State::error;result.error=GetLastError();return result;}
    DWORD got=0;
    BOOL ready=ReadFile(h,bytes,length,&got,&ov);
    if(!ready && GetLastError()==ERROR_IO_PENDING){
        DWORD wait=WaitForSingleObject(ov.hEvent,150);
        if(wait==WAIT_OBJECT_0)ready=GetOverlappedResult(h,&ov,&got,FALSE);
        else {CancelIoEx(h,&ov);GetOverlappedResult(h,&ov,&got,TRUE);result.state=State::error;result.error=ERROR_TIMEOUT;}
    }else if(!ready){result.state=State::error;result.error=GetLastError();}
    if(ready)result=sonyBattery(sense,bt,bytes,got);
    SecureZeroMemory(bytes,buffer.size());CloseHandle(ov.hEvent);
    if(allowExtended && bt && !sense && result.state==State::unsupported){
        // Explicit opt-in only. Official DS4 GET calibration 0x02 switches BT
        // to full input reports until reconnection; no calibration is written.
        std::vector<BYTE> calibration(std::max(37u,std::min(featureLength,4096u)),0);calibration[0]=0x02;
        BOOL ok=HidD_GetFeature(h,calibration.data(),ULONG(calibration.size()));DWORD error=ok?0:GetLastError();
        SecureZeroMemory(calibration.data(),calibration.size());
        if(!ok){result.state=State::error;result.error=error;return result;}
        for(int i=0;i<8;++i){result=readSony(h,length,sense,bt);if(result.state!=State::unsupported)break;}
    }
    return result;
}
inline void discoverSony(std::vector<Device>& found,const std::vector<std::wstring>& selected,bool readAll=false,const std::vector<std::wstring>& extended={}){
    GUID guid;HidD_GetHidGuid(&guid);
    HDEVINFO list=SetupDiGetClassDevsW(&guid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(list==INVALID_HANDLE_VALUE)return;
    for(DWORD i=0;;++i){
        SP_DEVICE_INTERFACE_DATA iface{};iface.cbSize=sizeof(iface);if(!SetupDiEnumDeviceInterfaces(list,nullptr,&guid,i,&iface))break;
        DWORD size=0;SetupDiGetDeviceInterfaceDetailW(list,&iface,nullptr,0,&size,nullptr);if(!size)continue;
        std::vector<BYTE> storage(size);auto detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());detail->cbSize=sizeof(*detail);
        SP_DEVINFO_DATA node{};node.cbSize=sizeof(node);
        if(!SetupDiGetDeviceInterfaceDetailW(list,&iface,detail,size,nullptr,&node))continue;
        std::wstring path=lower(detail->DevicePath);
        bool sense=path.find(L"vid&0002054c_pid&0ce6")!=path.npos || path.find(L"vid&0002054c_pid&0df2")!=path.npos || path.find(L"vid_054c&pid_0ce6")!=path.npos || path.find(L"vid_054c&pid_0df2")!=path.npos;
        bool ds4=path.find(L"vid&0002054c_pid&09cc")!=path.npos || path.find(L"vid&0002054c_pid&05c4")!=path.npos || path.find(L"vid_054c&pid_09cc")!=path.npos || path.find(L"vid_054c&pid_05c4")!=path.npos;
        if(!sense && !ds4)continue;
        HANDLE h=CreateFileW(detail->DevicePath,0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
        if(h==INVALID_HANDLE_VALUE)continue;
        PHIDP_PREPARSED_DATA pp=nullptr;HIDP_CAPS caps{};
        if(HidD_GetPreparsedData(h,&pp)){HidP_GetCaps(pp,&caps);HidD_FreePreparsedData(pp);}
        if(caps.UsagePage!=1 || (caps.Usage!=5 && caps.Usage!=4)){CloseHandle(h);continue;}
        wchar_t serial[128]{};HidD_GetSerialNumberString(h,serial,sizeof(serial));CloseHandle(h);
        std::wstring physical=compactSerial(serial);SecureZeroMemory(serial,sizeof(serial));
        wchar_t instance[1024]{};SetupDiGetDeviceInstanceIdW(list,&node,instance,1024,nullptr);
        bool bt=path.find(L"00001124-0000-1000-8000-00805f9b34fb")!=path.npos || path.find(L"vid&0002054c")!=path.npos;
        // Windows HID input length distinguishes these known full USB/BT collections.
        bt=caps.InputReportByteLength==78 || bt;
        GUID container{};property(list,node,DEVPKEY_Device_ContainerId,&container,sizeof(container));
        Device d;d.id=physical.empty()?L"sony-container:"+guidText(container):L"bt:"+physical;
        if(physical.empty() && container==GUID{})d.id=L"sony-path:"+path;
        if(physical.empty())for(const auto& known:found)if(known.container==guidText(container)){d.id=known.id;break;}
        d.defaultName=sense?L"DualSense":L"DualShock 4";d.kind=DeviceKind::controller;d.connection=bt?L"Bluetooth":L"USB";
        d.reading.state=State::unsupported;d.source=L"Sony passive HID battery";
        auto it=std::find_if(found.begin(),found.end(),[&](const Device& x){return x.id==d.id;});
        if(it!=found.end() && it->reading.state==State::reading)continue;
        bool enabled=readAll || std::find(selected.begin(),selected.end(),d.id)!=selected.end();
        if(enabled){
            h=CreateFileW(detail->DevicePath,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
            if(h!=INVALID_HANDLE_VALUE){d.reading=readSony(h,caps.InputReportByteLength,sense,bt,caps.FeatureReportByteLength,std::find(extended.begin(),extended.end(),d.id)!=extended.end());CloseHandle(h);if(d.reading.state==State::unsupported)d.source=L"Sony passive HID — full battery report unavailable";}
            else {d.reading.state=State::error;d.reading.error=GetLastError();}
        }
        if(it!=found.end())*it=d;else found.push_back(d);
    }
    SetupDiDestroyDeviceInfoList(list);
}
inline std::vector<Device> queryOtherDevices(const std::vector<std::wstring>& selected,bool readAll=false,const std::vector<std::wstring>& extended={}){
    auto found=discoverBluetooth();discoverSony(found,selected,readAll,extended);return found;
}
}
