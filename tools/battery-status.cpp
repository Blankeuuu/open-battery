#include "../src/hid.hpp"
#include <cstdio>
int main(){
    const auto r=ob::query();
    const char* state=r.state==ob::State::reading?"reading":r.state==ob::State::inactive?"inactive":r.state==ob::State::disconnected?"disconnected":"error";
    printf("{\"state\":\"%s\",\"percent\":%d,\"charging\":\"unknown\",\"win32_error\":%lu}\n",state,r.percent,r.error);
    return r.state==ob::State::reading?0:1;
}
