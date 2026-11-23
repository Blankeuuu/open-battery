#include "../src/model.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static int checks=0;
static void check(bool ok,const char* message){++checks;if(!ok){printf("FAIL: %s\n",message);exit(1);}}
int main(){
    // Captured local Windows report, active and inactive, with cached 100%.
    std::uint8_t r[65]{0,0,0,100,1,0,1,2};
    auto v=ob::decode(r,65);
    check(v.state==ob::State::reading && v.percent==100 && !v.charging,"local active status, charging unknown");
    r[3]=37; check(ob::decode(r,65).percent==37,"percentage is not hard-coded");
    r[5]=1; v=ob::decode(r,65);
    check(v.state==ob::State::inactive && v.percent==-1,"sleep never refreshes cached percentage");
    r[5]=0;r[3]=0;check(ob::decode(r,65).percent==0,"valid online empty battery");
    r[3]=101;check(ob::decode(r,65).state==ob::State::error,"out of range rejected");
    r[3]=37;r[0]=5;check(ob::decode(r,65).state==ob::State::error,"unexpected report ID rejected");
    r[0]=0;r[1]=0xf7;check(ob::decode(r,65).state==ob::State::error,"request echo rejected");
    r[1]=0;check(ob::decode(r,5).state==ob::State::error,"short reply rejected");
    std::memset(r,0,sizeof(r));check(ob::decode(r,65).state==ob::State::error,"all-zero reply rejected");
    ob::Alert a;ob::Reading reading{ob::State::reading,20,false,0};
    check(a.observe(reading),"first low reading warns");
    check(!a.observe(reading),"no repeated warning");
    ob::Alert restarted{a.warned};check(!restarted.observe(reading),"restart preserves warning");
    reading.percent=25;check(!a.observe(reading) && a.warned,"25 does not rearm");
    reading.percent=26;check(!a.observe(reading) && !a.warned,"26 rearms");
    reading.percent=15;reading.charging=true;check(!a.observe(reading) && !a.warned,"charging suppresses without consuming warning");
    reading.charging=false;check(a.observe(reading),"unplug low battery warns once");
    reading.state=ob::State::inactive;reading.percent=100;check(!a.observe(reading) && a.warned,"cached sleep reading cannot rearm");
    reading.state=ob::State::error;check(!a.observe(reading) && a.warned,"error cannot rearm");
    check(ob::batteryBand(0)==ob::BatteryBand::low,"0 is red band");
    check(ob::batteryBand(29)==ob::BatteryBand::low,"29 is red band");
    check(ob::batteryBand(30)==ob::BatteryBand::medium,"30 is orange band");
    check(ob::batteryBand(69)==ob::BatteryBand::medium,"69 is orange band");
    check(ob::batteryBand(70)==ob::BatteryBand::high,"70 is green band");
    check(ob::batteryBand(100)==ob::BatteryBand::high,"100 is green band");
    printf("PASS: %d model checks\n",checks);
}
