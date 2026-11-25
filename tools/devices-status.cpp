#include "../src/bluetooth.hpp"
#include "../src/device-settings.hpp"
#include <cstdio>
int wmain(int argc,wchar_t**){
    ULONGLONG start=GetTickCount64();
    std::vector<std::wstring> extended;for(const auto& d:ob::rememberedDevices())if(d.extendedReports)extended.push_back(d.id);
    auto devices=ob::queryOtherDevices({},true,extended);
    for(const auto& d:devices)wprintf(L"%ls | %ls | %ls | %ls | percent=%d approximate=%d charging=%ls error=%lu\n",d.defaultName.c_str(),ob::kindText(d.kind),d.connection.c_str(),ob::stateText(d.reading.state),d.reading.percent,d.reading.approximate,d.reading.chargingKnown?(d.reading.charging?L"yes":L"no"):L"unknown",d.reading.error);
    wprintf(L"Devices=%zu elapsed_ms=%llu\n",devices.size(),GetTickCount64()-start);
    if(argc>1){for(const auto& d:devices)if(d.kind==ob::DeviceKind::controller && d.connection==L"Bluetooth" && d.reading.state==ob::State::reading)return 0;puts("FAIL: connected Bluetooth controller has no battery reading");return 1;}
}
