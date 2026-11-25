#include "../src/ring-icon.hpp"
#include <cstdio>
static bool saveIcon(HICON icon,const wchar_t* path,int size){
    if(!icon)return false;ICONINFO ii{};GetIconInfo(icon,&ii);HDC dc=CreateCompatibleDC(nullptr);
    BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=size;bi.bmiHeader.biHeight=-size;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    std::vector<DWORD> pixels(size*size);GetDIBits(dc,ii.hbmColor,0,size,pixels.data(),&bi,DIB_RGB_COLORS);
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+size*size*4;
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);bool ok=file!=INVALID_HANDLE_VALUE;
    if(ok){DWORD n;WriteFile(file,&header,sizeof(header),&n,nullptr);WriteFile(file,&bi.bmiHeader,sizeof(bi.bmiHeader),&n,nullptr);WriteFile(file,pixels.data(),size*size*4,&n,nullptr);CloseHandle(file);}
    DeleteObject(ii.hbmColor);DeleteObject(ii.hbmMask);DeleteDC(dc);return ok;
}
int wmain(){
    CreateDirectoryW(L"build\\ring-icons",nullptr);
    for(bool dark:{false,true})for(int size:{16,20,24,32})for(int k=0;k<3;++k){
        ob::Device d;d.kind=ob::DeviceKind(k);d.reading={ob::State::reading,k==0?100:k==1?75:18};
        wchar_t path[160];swprintf(path,160,L"build\\ring-icons\\%ls-%d-%d-symbol.bmp",dark?L"dark":L"light",size,k);
        HICON icon=ob::ringIcon(d,size,dark);if(!saveIcon(icon,path,size))return 1;DestroyIcon(icon);
    }
    ob::unloadNumberFont();puts("Rendered 24 native colored-ring icons");
}
