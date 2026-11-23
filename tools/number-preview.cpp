#include "../src/number-icon.hpp"
#include <cstdio>
#include <string>
int wmain(){
    if(!ob::loadNumberFont())return 2;
    HDC dc=CreateCompatibleDC(nullptr);HFONT f=ob::digitFont(64);
    auto previous=SelectObject(dc,f);wchar_t face[64]{};GetTextFaceW(dc,64,face);
    wprintf(L"Selected font: %ls\n",face);
    bool correct=std::wstring(face)==L"JetBrains Mono ExtraBold";
    SelectObject(dc,previous);DeleteObject(f);DeleteDC(dc);
    if(!correct)return 3;
    CreateDirectoryW(L"build\\numbers",nullptr);
    for(bool dark : {false,true})for(int size : {16,20,24,32})for(int value : {100,70,69,30,29,0}){
        wchar_t text[8],file[160];swprintf(text,8,L"%d",value);
        swprintf(file,160,L"build\\numbers\\%ls-%d-%d.bmp",dark?L"dark":L"light",size,value);
        HICON icon=ob::numberIcon(text,ob::batteryColor(value,dark),size,file);
        if(!icon)return 4;DestroyIcon(icon);
    }
    ob::unloadNumberFont();return 0;
}
