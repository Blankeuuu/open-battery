#include "../src/wake-input.hpp"
#include <cstdio>
int main(){
 WNDCLASSW wc{};wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"OpenBattery.RawDeviceProbe";
 RegisterClassW(&wc);HWND h=CreateWindowExW(0,wc.lpszClassName,L"Raw device probe",0,0,0,0,0,nullptr,nullptr,wc.hInstance,nullptr);
 ob::WakeInput watcher;bool armed=watcher.arm(h);
 printf("AJ179 raw-input subscription=%s error=%lu\n",armed?"armed":"unavailable",watcher.error);
 watcher.stop();DestroyWindow(h);return armed?0:1;
}