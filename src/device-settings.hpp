#pragma once
#include <windows.h>
#include "devices.hpp"
namespace ob {
inline std::wstring deviceSettingsRoot=L"Software\\Open Battery\\Devices";
inline std::wstring deviceKey(const std::wstring& id){return deviceSettingsRoot+L"\\"+storageId(id);}
inline DWORD settingNumber(HKEY key,const wchar_t* name,DWORD fallback){
    DWORD value=0,n=sizeof(value);return RegGetValueW(key,nullptr,name,RRF_RT_REG_DWORD,nullptr,&value,&n)==ERROR_SUCCESS?value:fallback;
}
inline std::wstring settingText(HKEY key,const wchar_t* name){
    wchar_t value[1024]{};DWORD n=sizeof(value);
    return RegGetValueW(key,nullptr,name,RRF_RT_REG_SZ,nullptr,value,&n)==ERROR_SUCCESS?value:L"";
}
inline void textSetting(HKEY key,const wchar_t* name,const std::wstring& value){
    RegSetValueExW(key,name,0,REG_SZ,reinterpret_cast<const BYTE*>(value.c_str()),DWORD((value.size()+1)*sizeof(wchar_t)));
}
inline void numberSetting(HKEY key,const wchar_t* name,DWORD value){RegSetValueExW(key,name,0,REG_DWORD,reinterpret_cast<const BYTE*>(&value),sizeof(value));}
inline void saveDevice(const Device& d){
    HKEY key=nullptr;if(RegCreateKeyExW(HKEY_CURRENT_USER,deviceKey(d.id).c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS)return;
    textSetting(key,L"Id",d.id);textSetting(key,L"Name",d.defaultName);textSetting(key,L"Alias",d.alias);
    numberSetting(key,L"Kind",DWORD(d.kind));numberSetting(key,L"Enabled",d.enabled);numberSetting(key,L"Notifications",d.notifications);
    numberSetting(key,L"BlinkLow",d.blinkLow);numberSetting(key,L"RingColor",d.identityColor());numberSetting(key,L"ExtendedReports",d.extendedReports);numberSetting(key,L"Badge",d.badge);numberSetting(key,L"Warned",d.alert.warned);RegCloseKey(key);
}
inline void loadDevice(Device& d){
    HKEY key=nullptr;if(RegOpenKeyExW(HKEY_CURRENT_USER,deviceKey(d.id).c_str(),0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return;
    // Hash collisions must never merge settings for two physical devices.
    if(settingText(key,L"Id")!=d.id){RegCloseKey(key);return;}
    d.alias=settingText(key,L"Alias");d.enabled=settingNumber(key,L"Enabled",0)!=0;
    DWORD ring=settingNumber(key,L"RingColor",0xffffffffu);if(ring<=0xffffffu){d.ringColor=ring;d.customRingColor=true;}
    d.blinkLow=settingNumber(key,L"BlinkLow",1)!=0;d.extendedReports=settingNumber(key,L"ExtendedReports",0)!=0;d.notifications=settingNumber(key,L"Notifications",1)!=0;d.alert.warned=settingNumber(key,L"Warned",0)!=0;
    d.badge=std::min(settingNumber(key,L"Badge",0),DWORD(9));d.configured=true;RegCloseKey(key);
}
inline std::vector<Device> rememberedDevices(){
    std::vector<Device> result;HKEY root=nullptr;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,deviceSettingsRoot.c_str(),0,KEY_READ,&root)!=ERROR_SUCCESS)return result;
    for(DWORD i=0;;++i){wchar_t name[128];DWORD n=128;if(RegEnumKeyExW(root,i,name,&n,nullptr,nullptr,nullptr,nullptr)!=ERROR_SUCCESS)break;
        HKEY key=nullptr;if(RegOpenKeyExW(root,name,0,KEY_READ,&key)!=ERROR_SUCCESS)continue;
        Device d;d.id=settingText(key,L"Id");d.defaultName=settingText(key,L"Name");
        DWORD kind=settingNumber(key,L"Kind",3);d.kind=kind<=3?DeviceKind(kind):DeviceKind::other;
        RegCloseKey(key);if(d.id.empty())continue;loadDevice(d);d.reading.state=State::disconnected;result.push_back(d);
    }RegCloseKey(root);return result;
}
inline void forgetDevice(Device& d){
    RegDeleteTreeW(HKEY_CURRENT_USER,deviceKey(d.id).c_str());
    d.blinkLow=true;d.customRingColor=false;d.ringColor=0;d.extendedReports=false;d.enabled=false;d.notifications=true;d.alias.clear();d.badge=0;d.alert={};d.configured=false;d.lastRead=0;d.lastPercent=-1;
}
}
