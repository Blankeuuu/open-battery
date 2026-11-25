#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include "model.hpp"

namespace ob {
enum class DeviceKind { mouse, controller, headphones, other };
struct Device {
    std::wstring id, defaultName, alias, connection, source, container;
    DeviceKind kind=DeviceKind::other;
    Reading reading;
    int lastPercent=-1;
    bool lastApproximate=false;
    std::uint64_t lastRead=0;
    bool enabled=false, notifications=true, configured=false, extendedReports=false;
    unsigned badge=0;
    std::uint32_t ringColor=0;bool customRingColor=false;
    std::uint32_t identityColor() const {if(customRingColor)return ringColor;return kind==DeviceKind::mouse?0x005e56f4u:kind==DeviceKind::controller?0x00ff82b1u:kind==DeviceKind::headphones?0x00d2c643u:0x0046b7efu;}
    Alert alert;
    std::wstring name() const {return alias.empty()?defaultName:alias;}
    void observe(const Reading& value,std::uint64_t now){
        reading=value;
        if(value.state==State::reading){lastPercent=value.percent;lastApproximate=value.approximate;lastRead=now;}
    }
};
inline const wchar_t* kindText(DeviceKind k){
    switch(k){case DeviceKind::mouse:return L"Mouse";case DeviceKind::controller:return L"Controller";
    case DeviceKind::headphones:return L"Headphones";default:return L"Device";}
}
inline const wchar_t* stateText(State s){
    switch(s){case State::reading:return L"Connected";case State::inactive:return L"Inactive";
    case State::disconnected:return L"Disconnected";case State::unsupported:return L"Battery unavailable";
    default:return L"Could not read battery";}
}
inline std::wstring storageId(const std::wstring& id){
    // Stable per-user storage key; never expose hardware identifiers in diagnostics.
    std::uint64_t h=14695981039346656037ull;
    for(wchar_t c:id){h^=static_cast<std::uint16_t>(c);h*=1099511628211ull;}
    wchar_t b[24];swprintf(b,24,L"%016llx",static_cast<unsigned long long>(h));return b;
}
inline void mergeDevices(std::vector<Device>& devices,const std::vector<Device>& found,std::uint64_t now){
    for(auto& d:devices)if(d.id!=L"aj179")d.reading={State::disconnected};
    for(const auto& f:found){
        auto it=std::find_if(devices.begin(),devices.end(),[&](const Device& d){return d.id==f.id;});
        if(it==devices.end()){devices.push_back(f);it=devices.end()-1;}
        it->defaultName=f.defaultName;it->kind=f.kind;it->connection=f.connection;it->source=f.source;
        it->observe(f.reading,now);
    }
}
// Only the battery status byte is interpreted. Controls, sensors and touch data
// are neither retained nor logged. Report layouts documented in docs/bluetooth.md.
inline std::uint32_t sonyCrc(const std::uint8_t* bytes,std::size_t n){
    std::uint32_t crc=0xffffffffu;
    auto add=[&](std::uint8_t b){crc^=b;for(int i=0;i<8;++i)crc=(crc>>1)^(0xedb88320u&-(crc&1u));};
    add(0xa1);for(std::size_t i=0;i<n;++i)add(bytes[i]);return ~crc;
}
inline Reading sonyBattery(bool dualsense,bool bluetooth,const std::uint8_t* r,std::size_t n){
    Reading out;out.state=State::unsupported;
    const std::size_t expected=bluetooth?78:64;
    const std::uint8_t report=bluetooth?(dualsense?0x31:0x11):0x01;
    if(bluetooth && n>=expected && n<=4096)n=expected; // Windows pads BT to the maximum HID input length (DS4: 547).
    if(n!=expected || r[0]!=report)return out; // Reduced BT reports contain no battery.
    if(bluetooth){
        std::uint32_t got=0;for(int i=0;i<4;++i)got|=std::uint32_t(r[n-4+i])<<(8*i);
        if(sonyCrc(r,n-4)!=got){out.state=State::error;out.error=23;return out;}
    }
    const std::size_t pos=dualsense?(bluetooth?54:53):(bluetooth?32:30);
    const unsigned level=r[pos]&15,mode=r[pos]>>4;
    out.approximate=true;out.chargingKnown=true;
    if(dualsense){
        if(mode==2){out.percent=100;out.charging=false;}
        else if(mode<=1 && level<=10){out.percent=std::min(int(level)*10+5,100);out.charging=mode==1;}
        else {out.state=State::error;out.error=13;return out;}
    }else{
        const bool cable=(r[pos]&16)!=0;
        if(level>10 && !(cable && level==11)){out.state=State::error;out.error=13;return out;}
        out.percent=std::min(int(level)*10+5,100);out.charging=cable && level<=10;
    }
    out.state=State::reading;return out;
}
}
