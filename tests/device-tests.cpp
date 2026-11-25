#include "../src/devices.hpp"
#include <cstdio>
#include <cstdlib>
void check(bool ok,const char* name){if(!ok){printf("FAIL %s\n",name);exit(1);}}
void crc(std::vector<std::uint8_t>& r){auto value=ob::sonyCrc(r.data(),r.size()-4);for(int i=0;i<4;++i)r[r.size()-4+i]=std::uint8_t(value>>(8*i));}
int main(){
    using namespace ob;
    std::vector<std::uint8_t> usb(64),bt(78);usb[0]=1;usb[30]=4;
    auto r=sonyBattery(false,false,usb.data(),usb.size());check(r.state==State::reading && r.percent==45 && r.approximate && !r.charging,"DS4 USB approximate discharging");
    usb[30]=0x13;r=sonyBattery(false,false,usb.data(),64);check(r.percent==35 && r.charging && r.chargingKnown,"DS4 USB charging");
    usb[30]=0x1b;r=sonyBattery(false,false,usb.data(),64);check(r.percent==100 && !r.charging,"DS4 full is not charging");
    usb[30]=0x1f;check(sonyBattery(false,false,usb.data(),64).state==State::error,"DS4 invalid status never zero battery");
    bt[0]=0x11;bt[32]=7;crc(bt);r=sonyBattery(false,true,bt.data(),78);check(r.percent==75 && r.state==State::reading,"DS4 BT offset and CRC");
    bt[32]=8;check(sonyBattery(false,true,bt.data(),78).state==State::error,"corrupt BT rejected");
    bt.assign(78,0);bt[0]=0x11;bt[32]=6;crc(bt);bt.resize(547,0);check(sonyBattery(false,true,bt.data(),bt.size()).state==State::reading,"Windows 547-byte padded Bluetooth report carries battery");
    std::uint8_t minimal[10]={1};check(sonyBattery(false,true,minimal,10).state==State::unsupported,"minimal BT report has no invented percent");
    usb.assign(64,0);usb[0]=1;usb[53]=0x16;r=sonyBattery(true,false,usb.data(),64);check(r.percent==65 && r.charging,"DualSense USB offset");
    bt.assign(78,0);bt[0]=0x31;bt[54]=0x25;crc(bt);r=sonyBattery(true,true,bt.data(),78);check(r.percent==100 && !r.charging,"DualSense BT full");
    for(unsigned mode:{0xau,0xbu,0xfu}){usb[53]=mode<<4;check(sonyBattery(true,false,usb.data(),64).state==State::error,"DualSense battery faults");}
    Device a;a.id=L"one";a.alias=L"My pad";a.enabled=true;a.badge=2;a.alert.warned=true;a.observe({State::reading,35,false,0,true,true},123);
    Device b;b.id=L"two";b.defaultName=L"Headphones";b.enabled=true;
    std::vector<Device> list{a,b};Device fresh;fresh.id=L"one";fresh.defaultName=L"Controller";fresh.reading={State::reading,45,false,0,true,true};
    mergeDevices(list,{fresh},456);check(list.size()==2 && list[0].alias==L"My pad" && list[0].enabled && list[0].badge==2,"stable identity retains user preferences");
    check(list[0].lastPercent==45 && list[0].lastRead==456,"correct reading advances history");
    check(list[1].reading.state==State::disconnected,"missing device stays remembered and disconnected");
    fresh.reading={State::error};mergeDevices(list,{fresh},789);check(list[0].lastRead==456 && list[0].reading.percent==-1,"failure preserves only historical reading");
    list[0].alert={};list[1].alert={};check(list[0].alert.observe({State::reading,15}) && list[1].alert.observe({State::reading,10}),"alerts independent per device");
    check(!list[0].alert.observe({State::reading,15}),"reconnect does not duplicate warning");
    check(storageId(L"one")==storageId(L"one") && storageId(L"one")!=storageId(L"two"),"stable separate storage identities");
    puts("PASS: Sony USB/BT battery formats and CRC, approximation, charging faults, device identity/history, independent alerts");
}
