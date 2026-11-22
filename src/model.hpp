#pragma once
#include <cstddef>
#include <cstdint>

namespace ob {
enum class State { reading, inactive, disconnected, error };
struct Reading {
    State state = State::error;
    int percent = -1;
    bool charging = false; // Unknown for AJ179 Pro; never inferred from percentage.
    unsigned long error = 0;
};
// Windows HID adds an ID byte to this unnumbered 64-byte feature report.
// The status fields, not an arbitrary in-range byte, identify a valid reply.
inline Reading decode(const std::uint8_t* r, std::size_t n) {
    Reading out;
    if(n != 65 || r[0] != 0 || r[1] > 1 || r[2] > 100 ||
       r[3] > 100 || r[4] > 1 || r[5] > 1 || r[6] > 1) return out;
    bool nonzero=false;
    for(std::size_t i=1;i<7;++i) nonzero |= r[i]!=0;
    if(!nonzero) return out; // An empty/echo reply is not a 0% battery.
    out.state = r[5] == 0 ? State::reading : State::inactive;
    // A sleeping receiver can keep a cached percentage. Never mark it fresh.
    if(out.state == State::reading) out.percent=r[3];
    return out;
}
struct Alert {
    bool warned=false;
    bool observe(const Reading& r) {
        if(r.state!=State::reading || r.percent<0 || r.percent>100) return false;
        if(r.percent>25) warned=false;
        if(r.percent<=20 && !r.charging && !warned) {warned=true;return true;}
        return false;
    }
};
}
