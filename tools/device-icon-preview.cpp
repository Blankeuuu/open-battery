#include "../src/device-icon.hpp"
#include <cstdio>
bool save(HICON icon,const wchar_t* path,int size){
    if(!icon)return false;ICONINFO ii{};if(!GetIconInfo(icon,&ii))return false;
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=size;bi.bmiHeader.biHeight=-size;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    std::vector<DWORD> pixels(size*size);GetDIBits(dc,ii.hbmColor,0,size,pixels.data(),&bi,DIB_RGB_COLORS);
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+size*size*4;
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);bool ok=file!=INVALID_HANDLE_VALUE;
    if(ok){DWORD n;WriteFile(file,&header,sizeof(header),&n,nullptr);WriteFile(file,&bi.bmiHeader,sizeof(bi.bmiHeader),&n,nullptr);WriteFile(file,pixels.data(),size*size*4,&n,nullptr);CloseHandle(file);}
    DeleteObject(ii.hbmColor);DeleteObject(ii.hbmMask);DeleteDC(dc);return ok;
}
int wmain(){
    CreateDirectoryW(L"build\\device-icons",nullptr);
    for(bool dark:{false,true})for(int size:{16,20,24,32})for(int k=0;k<3;++k){
        ob::Device d;d.kind=ob::DeviceKind(k);d.reading={ob::State::reading,k==0?100:k==1?75:18};
        wchar_t path[160];swprintf(path,160,L"build\\device-icons\\%ls-%d-%d-symbol.bmp",dark?L"dark":L"light",size,k);
        HICON icon=ob::deviceIcon(d,size,dark,true);if(!save(icon,path,size))return 1;DestroyIcon(icon);
        swprintf(path,160,L"build\\device-icons\\%ls-%d-%d-number.bmp",dark?L"dark":L"light",size,k);
        icon=ob::numberIcon(std::to_wstring(d.reading.percent).c_str(),ob::batteryColor(d.reading.percent,dark),size);if(!save(icon,path,size))return 1;DestroyIcon(icon);
    }
    ob::unloadNumberFont();puts("Rendered 48 production C icon pairs by native Win32/GDI");
}
