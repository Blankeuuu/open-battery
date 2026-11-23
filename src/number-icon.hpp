#pragma once
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include "model.hpp"
namespace ob {
inline HANDLE numberFont=nullptr;
inline bool loadNumberFont(){
    if(numberFont)return true;
    HMODULE module=GetModuleHandleW(nullptr);
    HRSRC resource=FindResourceW(module,MAKEINTRESOURCEW(200),MAKEINTRESOURCEW(10));
    if(!resource)return false;
    DWORD size=SizeofResource(module,resource),count=0;
    auto data=LockResource(LoadResource(module,resource));
    numberFont=AddFontMemResourceEx(data,size,nullptr,&count);
    return numberFont && count>0;
}
inline void unloadNumberFont(){if(numberFont){RemoveFontMemResourceEx(numberFont);numberFont=nullptr;}}
inline COLORREF batteryColor(int percent,bool dark){
    switch(batteryBand(percent)){
    case BatteryBand::high:return dark?RGB(85,217,139):RGB(22,138,69);
    case BatteryBand::medium:return dark?RGB(255,180,75):RGB(180,98,0);
    default:return dark?RGB(255,105,105):RGB(200,45,55);
    }
}
inline HFONT digitFont(int height){
    return CreateFontW(-height,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"JetBrains Mono");
}
inline HICON numberIcon(const wchar_t* text,COLORREF color,int size,const wchar_t* save=nullptr){
    if(!loadNumberFont())return nullptr;
    const int scale=4,w=size*12,h=size*6;
    HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    BITMAPINFO sourceInfo{};sourceInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    sourceInfo.bmiHeader.biWidth=w;sourceInfo.bmiHeader.biHeight=-h;
    sourceInfo.bmiHeader.biPlanes=1;sourceInfo.bmiHeader.biBitCount=32;
    void* raw=nullptr;
    HBITMAP source=CreateDIBSection(screen,&sourceInfo,DIB_RGB_COLORS,&raw,nullptr,0);
    if(!source){DeleteDC(dc);ReleaseDC(nullptr,screen);return nullptr;}
    auto old=SelectObject(dc,source);
    std::fill_n(static_cast<DWORD*>(raw),w*h,0);
    HFONT font=digitFont(size*scale);auto oldFont=SelectObject(dc,font);
    SetBkColor(dc,RGB(0,0,0));SetTextColor(dc,RGB(255,255,255));
    SetTextCharacterExtra(dc,-scale); // Tighten monospaced digits by one output pixel.
    TextOutW(dc,4,4,text,static_cast<int>(wcslen(text)));
    GdiFlush();
    auto src=static_cast<DWORD*>(raw);
    int left=w,top=h,right=-1,bottom=-1;
    for(int y=0;y<h;++y)for(int x=0;x<w;++x)if(src[y*w+x]&0xffffff){
        left=(std::min)(left,x);right=(std::max)(right,x);
        top=(std::min)(top,y);bottom=(std::max)(bottom,y);
    }
    BITMAPINFO outInfo=sourceInfo;outInfo.bmiHeader.biWidth=size;outInfo.bmiHeader.biHeight=-size;
    void* outRaw=nullptr;
    HBITMAP output=CreateDIBSection(screen,&outInfo,DIB_RGB_COLORS,&outRaw,nullptr,0);
    if(!output){SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,old);DeleteObject(source);DeleteDC(dc);ReleaseDC(nullptr,screen);return nullptr;}
    auto dst=static_cast<DWORD*>(outRaw);std::fill_n(dst,size*size,0);
    if(right>=left && bottom>=top){
        int sw=right-left+1,sh=bottom-top+1;
        // Keep digit height stable; condense three digits to fit the tiny tray cell.
        int th=(std::max)(1,size-4);
        int tw=(std::max)(1,(std::min)(size-2,int(std::round(double(sw)*th/sh))));
        int ox=(size-tw)/2,oy=(size-th)/2;
        for(int y=0;y<th;++y)for(int x=0;x<tw;++x){
            double x0=left+double(x)*sw/tw,x1=left+double(x+1)*sw/tw;
            double y0=top+double(y)*sh/th,y1=top+double(y+1)*sh/th,total=0;
            for(int sy=int(y0);sy<int(std::ceil(y1));++sy)
                for(int sx=int(x0);sx<int(std::ceil(x1));++sx){
                    double area=((std::min)(x1,double(sx+1))-(std::max)(x0,double(sx)))*
                        ((std::min)(y1,double(sy+1))-(std::max)(y0,double(sy)));
                    total+=(src[sy*w+sx]&255)*area;
                }
            DWORD a=static_cast<DWORD>(std::round(total/((x1-x0)*(y1-y0))));
            dst[(oy+y)*size+ox+x]=(a<<24)|((GetRValue(color)*a/255)<<16)|
                ((GetGValue(color)*a/255)<<8)|(GetBValue(color)*a/255);
        }
    }
    if(save){
        BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);
        file.bfSize=file.bfOffBits+size*size*4;
        HANDLE f=CreateFileW(save,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,0,nullptr);
        if(f!=INVALID_HANDLE_VALUE){DWORD n;WriteFile(f,&file,sizeof(file),&n,nullptr);
            WriteFile(f,&outInfo.bmiHeader,sizeof(BITMAPINFOHEADER),&n,nullptr);
            WriteFile(f,dst,size*size*4,&n,nullptr);CloseHandle(f);}
    }
    std::vector<BYTE> maskBits(((size+15)/16)*2*size,0);
    HBITMAP mask=CreateBitmap(size,size,1,1,maskBits.data());
    ICONINFO ii{};ii.fIcon=TRUE;ii.hbmColor=output;ii.hbmMask=mask;
    HICON result=CreateIconIndirect(&ii);
    DeleteObject(mask);DeleteObject(output);SelectObject(dc,oldFont);DeleteObject(font);
    SelectObject(dc,old);DeleteObject(source);DeleteDC(dc);ReleaseDC(nullptr,screen);
    return result;
}
}
