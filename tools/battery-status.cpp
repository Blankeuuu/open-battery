#include "../src/hid.hpp"
#include <cstdio>
#include <cstring>
int main(int argc,char** argv){
    const auto r=ob::query();
    const char* state=r.state==ob::State::reading?"reading":r.state==ob::State::inactive?"inactive":r.state==ob::State::disconnected?"disconnected":"error";
    const char* charge=r.chargingKnown?(r.charging?"yes":"no"):"unknown";
    printf("{\"state\":\"%s\",\"percent\":%d,\"charging\":\"%s\",\"win32_error\":%lu}\n",state,r.percent,charge,r.error);
    if(argc>1 && std::strcmp(argv[1],"--require-charging")==0)return r.chargingKnown&&r.charging?0:1;
    return r.state==ob::State::reading?0:1;
}